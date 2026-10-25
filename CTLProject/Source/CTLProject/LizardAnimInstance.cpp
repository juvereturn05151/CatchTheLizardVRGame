#include "LizardAnimInstance.h"
#include "LizardGameplayActor.h"
#include "Animation/AnimSequence.h"
#if WITH_EDITOR
#include "Animation/AnimBlueprint.h"
#include "AnimGraphNode_Root.h"
#include "AnimGraphNode_SequencePlayer.h"
#include "AnimGraphNode_TwoWayBlend.h"
#include "K2Node_VariableGet.h"
#include "EdGraphSchema_K2.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#endif

void ULizardAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds);
    const auto* Lizard = Cast<ALizardGameplayActor>(GetOwningActor());
    if (!Lizard) { SurfaceSpeed = MovementBlend = 0; bLocomotionAllowed = false; return; }
    IdleClip = Lizard->IdleAnimation; MovementClip = Lizard->MovementAnimation;
    bLocomotionAllowed = Lizard->CanPlaySurfaceLocomotion();
    SurfaceSpeed = bLocomotionAllowed ? Lizard->SurfaceLocomotionSpeed : 0.f;
    const float Target = bLocomotionAllowed && SurfaceSpeed > Lizard->AnimationMovementThreshold ? 1.f : 0.f;
    // Incapacitation immediately stops the crawl; ordinary starts/stops blend smoothly.
    MovementBlend = bLocomotionAllowed ? FMath::FInterpConstantTo(MovementBlend, Target, DeltaSeconds, 1.f / FMath::Max(.001f, Lizard->AnimationBlendSeconds)) : 0.f;
    MovementPlayRate = FMath::Clamp(SurfaceSpeed / FMath::Max(.1f, Lizard->AnimationReferenceSpeed), Lizard->AnimationMinPlayRate,
        FMath::Max(Lizard->AnimationMinPlayRate, Lizard->AnimationMaxPlayRate));
}

bool ULizardAnimInstance::AuthorAnimationGraph(UAnimBlueprint* Blueprint, UAnimSequence* Idle, UAnimSequence* Movement)
{
#if WITH_EDITOR
    if (!Blueprint || !Idle || !Movement) return false;
    UEdGraph* Graph = nullptr;
    for (UEdGraph* Candidate : Blueprint->FunctionGraphs) if (Candidate && Candidate->GetFName() == TEXT("AnimGraph")) Graph = Candidate;
    if (!Graph) return false;
    Blueprint->Modify(); Graph->Modify();
    UAnimGraphNode_Root* Root = nullptr;
    TArray<UEdGraphNode*> Previous;
    for (UEdGraphNode* Node : Graph->Nodes) { if (auto* Output = Cast<UAnimGraphNode_Root>(Node)) Root = Output; else Previous.Add(Node); }
    if (!Root) return false;
    for (auto* Node : Previous) Graph->RemoveNode(Node);
    const auto Add = [Graph](UClass* Class, int32 X, int32 Y) {
        auto* Node = NewObject<UEdGraphNode>(Graph, Class, NAME_None, RF_Transactional);
        Graph->AddNode(Node, false, false); Node->CreateNewGuid(); Node->PostPlacedNewNode(); Node->AllocateDefaultPins();
        Node->NodePosX = X; Node->NodePosY = Y; return Node;
    };
    auto* IdlePlayer = CastChecked<UAnimGraphNode_SequencePlayer>(Add(UAnimGraphNode_SequencePlayer::StaticClass(), -650, -180));
    auto* MovePlayer = CastChecked<UAnimGraphNode_SequencePlayer>(Add(UAnimGraphNode_SequencePlayer::StaticClass(), -650, 140));
    IdlePlayer->SetAnimationAsset(Idle); MovePlayer->SetAnimationAsset(Movement);
    const auto Expose = [](UAnimGraphNode_Base* Node, FName Property) {
        for (auto& Optional : Node->ShowPinForProperties) if (Optional.PropertyName == Property) Optional.bShowPin = true;
        Node->ReconstructNode();
    };
    Expose(IdlePlayer, TEXT("Sequence")); Expose(MovePlayer, TEXT("Sequence")); Expose(MovePlayer, TEXT("PlayRate"));
    auto* Blend = CastChecked<UAnimGraphNode_TwoWayBlend>(Add(UAnimGraphNode_TwoWayBlend::StaticClass(), -150, 0));
    Expose(Blend, TEXT("Alpha"));
    const auto* Schema = GetDefault<UEdGraphSchema_K2>();
    const auto Link = [Schema](UEdGraphPin* A, UEdGraphPin* B) { return A && B && Schema->TryCreateConnection(A, B); };
    const auto Variable = [&](FName Name, int32 Y, UEdGraphPin* Destination) {
        auto* Node = CastChecked<UK2Node_VariableGet>(Add(UK2Node_VariableGet::StaticClass(), -1000, Y));
        Node->VariableReference.SetSelfMember(Name); Node->ReconstructNode(); return Link(Node->FindPin(Name), Destination);
    };
    bool Good = Variable(TEXT("IdleClip"), -280, IdlePlayer->FindPin(TEXT("Sequence")));
    Good &= Variable(TEXT("MovementClip"), 100, MovePlayer->FindPin(TEXT("Sequence")));
    Good &= Variable(TEXT("MovementPlayRate"), 280, MovePlayer->FindPin(TEXT("PlayRate")));
    Good &= Variable(TEXT("MovementBlend"), 440, Blend->FindPin(TEXT("Alpha")));
    Good &= Link(IdlePlayer->FindPin(TEXT("Pose")), Blend->FindPin(TEXT("A")));
    Good &= Link(MovePlayer->FindPin(TEXT("Pose")), Blend->FindPin(TEXT("B")));
    Good &= Link(Blend->FindPin(TEXT("Pose")), Root->FindPin(TEXT("Result")));
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
    FKismetEditorUtilities::CompileBlueprint(Blueprint);
    return Good && Blueprint->Status == BS_UpToDate;
#else
    return false;
#endif
}


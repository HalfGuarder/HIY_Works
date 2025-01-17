// Fill out your copyright notice in the Description page of Project Settings.


#include "TFT_TeamAI_Knight.h"

#include "Components/SphereComponent.h"
#include "Math/UnrealMathUtility.h"
#include "Engine/DamageEvents.h"

#include "TFT_HpBar.h"
#include "Components/WidgetComponent.h"
#include "Blueprint/UserWidget.h"

#include "TFT_AnimInstance_Knight.h"

#include "TFT_GameInstance.h"
#include "TFT_SoundManager.h"

#include "Kismet/GameplayStatics.h"

ATFT_TeamAI_Knight::ATFT_TeamAI_Knight()
{
	PrimaryActorTick.bCanEverTick = true;

	_meshCom = CreateDefaultSubobject<UTFT_MeshComponent>(TEXT("Mesh_Com"));


	SetMesh("/ Script / Engine.SkeletalMesh'/Game/ParagonTerra/Characters/Heroes/Terra/Skins/MountainForge/Meshes/Terra_MountainForge.Terra_MountainForge'");

	_hpbarWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("HpBar"));

	_hpbarWidget->SetupAttachment(GetMesh());
	_hpbarWidget->SetWidgetSpace(EWidgetSpace::Screen);
	_hpbarWidget->SetRelativeLocation(FVector(0.0f, 0.0f, 300.0f));


	static ConstructorHelpers::FClassFinder<UUserWidget> hpBar(TEXT("/Script/UMGEditor.WidgetBlueprint'/Game/BluePrint/UI/TFT_HpBar_Nomal_BP.TFT_HpBar_Nomal_BP_C'"));
	if (hpBar.Succeeded())
	{
		_hpbarWidget->SetWidgetClass(hpBar.Class);
	}

	SetMesh("/Script/Engine.SkeletalMesh'/Game/ParagonTerra/Characters/Heroes/Terra/Skins/MountainForge/Meshes/Terra_MountainForge.Terra_MountainForge'");

	
}

void ATFT_TeamAI_Knight::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 플레이어 캐릭터 가져오기
	AActor* Player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);

	if (Player)
	{
		// 플레이어와 AI 캐릭터 간 거리 계산
		float Distance = FVector::Dist(Player->GetActorLocation(), GetActorLocation());

		// 체력바 위젯이 존재하는지 확인
		if (_hpbarWidget)
		{
			// 특정 거리 내에 있을 경우 체력바 표시, 멀리 있으면 숨김
			if (Distance <= 700.0f)  // 예: 1000 유닛 이내일 때 체력바 표시
			{
				_hpbarWidget->SetVisibility(true);
				_hpbarWidget->GetUserWidgetObject()->SetVisibility(ESlateVisibility::Visible);
			}
			else
			{
				_hpbarWidget->SetVisibility(false);
				_hpbarWidget->GetUserWidgetObject()->SetVisibility(ESlateVisibility::Hidden);
			}
		}
	}
}

void ATFT_TeamAI_Knight::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	_animInstance_Knight = Cast<UTFT_AnimInstance_Knight>(GetMesh()->GetAnimInstance());

	if (_animInstance_Knight->IsValidLowLevel())
	{
		_animInstance_Knight->OnMontageEnded.AddDynamic(this, &ATFT_Creature::OnAttackEnded);
		_animInstance_Knight->_attackStartDelegate.AddUObject(this, &ATFT_TeamAI_Knight::AttackStart);
		_animInstance_Knight->_attackHitDelegate.AddUObject(this, &ATFT_TeamAI_Knight::AttackHit);
		_animInstance_Knight->_deathStartDelegate.AddUObject(this, &ATFT_TeamAI_Knight::DeathStart);
		_animInstance_Knight->_deathEndDelegate.AddUObject(this, &ATFT_TeamAI_Knight::Disable);
	}

	
	_hpbarWidget->InitWidget();
	auto hpBar = Cast<UTFT_HpBar>(_hpbarWidget->GetUserWidgetObject());

	if (hpBar)
	{
		_statCom->_hpChangedDelegate.AddUObject(hpBar, &UTFT_HpBar::SetHpBarValue);
	}

	UE_LOG(LogTemp, Error, TEXT("AI_Knight... hp : %d, attackDamage : %d"), _statCom->GetMaxHp(), _statCom->GetAttackDamage());
}

void ATFT_TeamAI_Knight::BeginPlay()
{
	Super::BeginPlay();

	Init();

	_statCom->SetLevelAndInit(21);
}

void ATFT_TeamAI_Knight::Init()
{
}

void ATFT_TeamAI_Knight::SetMesh(FString path)
{
	_meshCom->SetMesh(path);
}

void ATFT_TeamAI_Knight::Attack_AI()
{
	Super::Attack_AI();

	if (_isAttacking == false && _animInstance_Knight != nullptr)
	{
		_animInstance_Knight->PlayAttackMontage();
		_isAttacking = true;

		_curAttackIndex %= 3;
		_curAttackIndex++;

		_animInstance_Knight->JumpToSection(_curAttackIndex);
	}
}

void ATFT_TeamAI_Knight::AttackStart()
{
	Super::AttackStart();

	SoundManager->Play("TeamAI_Knight_Swing", GetActorLocation());
}

void ATFT_TeamAI_Knight::AttackHit()
{
	FHitResult hitResult;
	FCollisionQueryParams params(NAME_None, false, this);

	float attackRange = 200.0f;
	float attackRadius = 100.0f;

	bool bResult = GetWorld()->SweepSingleByChannel
	(
		hitResult,
		GetActorLocation(),
		GetActorLocation() + GetActorForwardVector() * attackRange,
		FQuat::Identity,
		ECollisionChannel::ECC_GameTraceChannel3,
		FCollisionShape::MakeSphere(attackRadius),
		params
	);

	FVector vec = GetActorForwardVector() * attackRange;
	FVector center = GetActorLocation() + vec * 0.5f;

	FColor drawColor = FColor::Green;

	if (bResult && hitResult.GetActor()->IsValidLowLevel())
	{
		drawColor = FColor::Red;
		FDamageEvent damageEvent;
		hitResult.GetActor()->TakeDamage(_statCom->GetAttackDamage(), damageEvent, GetController(), this);
		

	}

}

void ATFT_TeamAI_Knight::DeathStart()
{
	Super::DeathStart();

	SoundManager->Play("TeamAI_Knight_Death", GetActorLocation());

	_animInstance_Knight->_deathStartDelegate.RemoveAll(this);
}

void ATFT_TeamAI_Knight::Disable()
{
	Super::Disable();

	_animInstance_Knight->_deathEndDelegate.RemoveAll(this);
}

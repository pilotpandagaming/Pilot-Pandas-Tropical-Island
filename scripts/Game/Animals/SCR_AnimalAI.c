class SCR_AnimalAI: ScriptedEntityComponent
{
	// Animal behavior states
	protected enum EAnimalState
	{
		IDLE,
		WANDERING,
		FLEEING,
		EATING,
		SLEEPING
	}
	
	// Configuration attributes
	[Attribute("Animal Type", "The type of animal (e.g., 'Deer', 'Bird', 'Fish')")]
	protected string m_sAnimalType;
	
	[Attribute("Movement Speed", "How fast the animal moves (m/s)", "1.0")]
	protected float m_fMovementSpeed;
	
	[Attribute("Detection Range", "How far the animal can detect threats (m)", "20.0")]
	protected float m_fDetectionRange;
	
	[Attribute("Flee Speed Multiplier", "Speed multiplier when fleeing", "2.0")]
	protected float m_fFleeSpeedMultiplier;
	
	[Attribute("Wander Radius", "Maximum distance from spawn point (m)", "50.0")]
	protected float m_fWanderRadius;
	
	[Attribute("Idle Time Range", "Random idle time range in seconds", "5.0 15.0")]
	protected vector2 m_vIdleTimeRange;
	
	// Internal variables
	protected EAnimalState m_eCurrentState;
	protected vector m_vSpawnPosition;
	protected vector m_vTargetPosition;
	protected float m_fStateTimer;
	protected float m_fNextStateTime;
	protected IEntity m_pOwner;
	protected SCR_AIWorld m_pAIWorld;
	protected SCR_AICombatComponent m_pCombatComponent;
	protected SCR_AIMovementComponent m_pMovementComponent;
	
	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		m_pOwner = owner;
		m_vSpawnPosition = owner.GetOrigin();
		m_eCurrentState = EAnimalState.IDLE;
		
		// Get required components
		m_pAIWorld = SCR_AIWorld.Cast(GetGame().GetWorld().FindEntityByClass(SCR_AIWorld));
		m_pCombatComponent = SCR_AICombatComponent.Cast(owner.FindComponent(SCR_AICombatComponent));
		m_pMovementComponent = SCR_AIMovementComponent.Cast(owner.FindComponent(SCR_AIMovementComponent));
		
		// Initialize movement component if available
		if (m_pMovementComponent)
		{
			m_pMovementComponent.SetMovementSpeed(m_fMovementSpeed);
		}
		
		// Set initial state timer
		SetNextStateTime();
		
		// Start the AI update loop
		GetGame().GetCallqueue().CallLater(UpdateAI, 100, true);
	}
	
	//------------------------------------------------------------------------------------------------
	protected void UpdateAI()
	{
		if (!m_pOwner || !m_pOwner.IsAlive())
			return;
			
		// Update state timer
		m_fStateTimer += 0.1; // 100ms intervals
		
		// Check for threats
		if (CheckForThreats())
		{
			SetState(EAnimalState.FLEEING);
			return;
		}
		
		// Update current state
		switch (m_eCurrentState)
		{
			case EAnimalState.IDLE:
				UpdateIdleState();
				break;
			case EAnimalState.WANDERING:
				UpdateWanderingState();
				break;
			case EAnimalState.FLEEING:
				UpdateFleeingState();
				break;
			case EAnimalState.EATING:
				UpdateEatingState();
				break;
			case EAnimalState.SLEEPING:
				UpdateSleepingState();
				break;
		}
	}
	
	//------------------------------------------------------------------------------------------------
	protected void UpdateIdleState()
	{
		if (m_fStateTimer >= m_fNextStateTime)
		{
			// Randomly choose next state
			int randomState = Math.RandomInt(0, 3);
			switch (randomState)
			{
				case 0:
					SetState(EAnimalState.WANDERING);
					break;
				case 1:
					SetState(EAnimalState.EATING);
					break;
				case 2:
					SetState(EAnimalState.SLEEPING);
					break;
				default:
					SetState(EAnimalState.WANDERING);
					break;
			}
		}
	}
	
	//------------------------------------------------------------------------------------------------
	protected void UpdateWanderingState()
	{
		// Check if we've reached the target position
		vector currentPos = m_pOwner.GetOrigin();
		float distanceToTarget = vector.Distance(currentPos, m_vTargetPosition);
		
		if (distanceToTarget < 2.0 || m_fStateTimer >= m_fNextStateTime)
		{
			SetState(EAnimalState.IDLE);
			return;
		}
		
		// Move towards target position
		if (m_pMovementComponent)
		{
			vector direction = m_vTargetPosition - currentPos;
			direction.Normalize();
			m_pMovementComponent.SetMovementDirection(direction);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	protected void UpdateFleeingState()
	{
		// Check if we're far enough from threat or if flee time is up
		if (m_fStateTimer >= 10.0) // Flee for 10 seconds
		{
			SetState(EAnimalState.IDLE);
			return;
		}
		
		// Continue fleeing away from last known threat position
		if (m_pMovementComponent)
		{
			vector currentPos = m_pOwner.GetOrigin();
			vector fleeDirection = currentPos - m_vSpawnPosition;
			fleeDirection.Normalize();
			m_pMovementComponent.SetMovementDirection(fleeDirection);
			m_pMovementComponent.SetMovementSpeed(m_fMovementSpeed * m_fFleeSpeedMultiplier);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	protected void UpdateEatingState()
	{
		if (m_fStateTimer >= m_fNextStateTime)
		{
			SetState(EAnimalState.IDLE);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	protected void UpdateSleepingState()
	{
		if (m_fStateTimer >= m_fNextStateTime)
		{
			SetState(EAnimalState.IDLE);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	protected bool CheckForThreats()
	{
		if (!m_pAIWorld)
			return false;
			
		vector currentPos = m_pOwner.GetOrigin();
		
		// Get all entities in detection range
		array<IEntity> entities = new array<IEntity>();
		m_pAIWorld.GetEntitiesInRadius(currentPos, m_fDetectionRange, entities);
		
		foreach (IEntity entity : entities)
		{
			// Check if entity is a player or vehicle
			if (entity.IsInherited(ChimeraCharacter) || entity.IsInherited(CarScript))
			{
				// Check if entity is moving towards us
				vector entityPos = entity.GetOrigin();
				vector directionToEntity = entityPos - currentPos;
				directionToEntity.Normalize();
				
				// Simple threat detection - if entity is close and moving
				if (vector.Distance(currentPos, entityPos) < m_fDetectionRange * 0.5)
				{
					return true;
				}
			}
		}
		
		return false;
	}
	
	//------------------------------------------------------------------------------------------------
	protected void SetState(EAnimalState newState)
	{
		m_eCurrentState = newState;
		m_fStateTimer = 0.0;
		SetNextStateTime();
		
		// Handle state-specific initialization
		switch (newState)
		{
			case EAnimalState.WANDERING:
				SetRandomTargetPosition();
				break;
			case EAnimalState.FLEEING:
				// Reset movement speed to normal after fleeing
				if (m_pMovementComponent)
				{
					m_pMovementComponent.SetMovementSpeed(m_fMovementSpeed);
				}
				break;
		}
	}
	
	//------------------------------------------------------------------------------------------------
	protected void SetRandomTargetPosition()
	{
		// Generate random position within wander radius
		float randomAngle = Math.RandomFloat(0, 2 * Math.PI);
		float randomDistance = Math.RandomFloat(5, m_fWanderRadius);
		
		vector randomOffset = vector(
			Math.Cos(randomAngle) * randomDistance,
			0,
			Math.Sin(randomAngle) * randomDistance
		);
		
		m_vTargetPosition = m_vSpawnPosition + randomOffset;
	}
	
	//------------------------------------------------------------------------------------------------
	protected void SetNextStateTime()
	{
		m_fNextStateTime = Math.RandomFloat(m_vIdleTimeRange[0], m_vIdleTimeRange[1]);
	}
	
	//------------------------------------------------------------------------------------------------
	// Public methods for external control
	void SetAnimalType(string animalType)
	{
		m_sAnimalType = animalType;
	}
	
	string GetAnimalType()
	{
		return m_sAnimalType;
	}
	
	EAnimalState GetCurrentState()
	{
		return m_eCurrentState;
	}
	
	void ForceFlee()
	{
		SetState(EAnimalState.FLEEING);
	}
}; 
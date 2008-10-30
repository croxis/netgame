/*
	Authority Management Example
	From "Networking for Game Programmers" - http://www.gaffer.org/networking-for-game-programmers
	Author: Glenn Fiedler <gaffer@gaffer.org>
*/

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "ode/ode.h"
#include "Platform.h"
#include "Display.h"
//#include "Transport.h"

using namespace std;
using namespace net;

// ------------------------------------------------------------------------------

// abstracted simulation data (decouple sim and networking)

struct SimulationPlayerInput
{
	SimulationPlayerInput()
	{
		left = false;
		right = false;
		forward = false;
		back = false;
	}
	
	bool left;
	bool right;
	bool forward;
	bool back;
};

struct SimulationPlayerState
{
	SimulationPlayerState()
	{
		cubeId = -1;
	}
	
	int cubeId;
};

struct SimulationCubeState
{
	SimulationCubeState()
	{
		enabled = false;
		position = math::Vector(0,0,0);
		linearVelocity = math::Vector(0,0,0);
		angularVelocity = math::Vector(0,0,0);
		orientation = math::Vector(1,0,0,0);
	}
	
	bool enabled;
 	math::Vector position;
 	math::Vector linearVelocity;
 	math::Vector angularVelocity;
 	math::Quaternion orientation;	
};

struct SimulationCubeContacts
{
	SimulationCubeContacts()
	{
		memset( this, 0, sizeof( SimulationCubeContacts ) );
	}
	
	bool touching[MaxCubes];
};

struct SimulationPlayerInteractions
{
	SimulationPlayerInteractions()
	{
		memset( this, 0, sizeof( SimulationCubeContacts ) );
	}
	
	bool interacting[MaxCubes];
};

struct SimulationState
{
	SimulationState()
	{
		for ( int i = 0; i < MaxPlayers; ++i )
			playerExists[i] = false;
		numCubes = 0;
	}

	bool playerExists[MaxPlayers];

	SimulationPlayerInput playerInput[MaxPlayers];
	SimulationPlayerState playerState[MaxPlayers];

	int numCubes;	
	SimulationCubeState cubeState[MaxCubes];
	SimulationCubeContacts cubeContacts[MaxCubes];
	SimulationPlayerInteractions playerInteractions[MaxPlayers];
	
	bool attachmentLastFrame;
};

/*
// physics simulation (ODE)

class PhysicsSimulation
{	
public:
	
	PhysicsSimulation()
	{
		// create simulation

		world = dWorldCreate();
		space = dHashSpaceCreate( 0 );
	    contacts = dJointGroupCreate( 0 );
		
		// configure world
		
		dWorldSetERP( world, ERP );
		dWorldSetCFM( world, CFM );
		dWorldSetQuickStepNumIterations( world, MaxIterations );
		dWorldSetGravity( world, 0, -Gravity, 0 );
		dWorldSetContactSurfaceLayer( world, ContactSurfaceLayer );
		dWorldSetContactMaxCorrectingVel( world, MaximumCorrectingVelocity );

		// add boundary planes
		
		dCreatePlane( space, 0, 1, 0, 0 );						// floor
		dCreatePlane( space, +1, 0, 0, -Boundary );				// left wall
		dCreatePlane( space, -1, 0, 0, -Boundary );				// right wall
		dCreatePlane( space, 0, 0, +1, -Boundary );				// front wall
		dCreatePlane( space, 0, 0, -1, -Boundary );				// back wall
		
		// add some cubes to the world

		#if DEMO == DEMO_STACKS

			// stacks

			AddCube( PblVector3(-3,2,0), 1 );
			AddCube( PblVector3(+3,2,0), 1 );

			AddCube( PblVector3(-3,1,0), 1 );
			AddCube( PblVector3(-3,10,0), 1 );

			AddCube( PblVector3(+3,1,0), 1 );
			AddCube( PblVector3(+3,10,0), 1 );
		
		#endif

		#if DEMO == DEMO_DYNAMIC_WORLD || DEMO == DEMO_AUTHORITY_MANAGEMENT
		
			// random
		
			srand( 280090412 );

			AddCube( PblVector3(-5,10,0), 1 );
			AddCube( PblVector3(+5,10,0), 1 );

			AddCube( PblVector3(0,1,0), 0.9f );
			AddCube( PblVector3(0,4,0), 0.9f );
			AddCube( PblVector3(0,8,0), 0.9f );

			for ( int i = 0; i < 30; ++i )
			{
				float x = random_float( -Boundary, +Boundary );
				float y = random_float( 1.0f, 4.0f );
				float z = random_float( -Boundary, +Boundary );
				float scale = random_float( 0.1f, 0.6f );

				AddCube( PblVector3(x,y,z), scale );
			}
			
			srand( 100 );

			for ( int i = 0; i < 29; ++i )
			{
				float x = random_float( -Boundary, +Boundary );
				float y = random_float( 2.0f, 4.0f );
				float z = random_float( -Boundary, +Boundary );
				float scale = random_float( 0.1f, 0.2f );

				AddCube( PblVector3(x,y,z), scale );
			}
			
		#endif

		#if DEMO == DEMO_KATAMARI
		
			// katamari!
		
			srand( 280090412 );

			AddCube( PblVector3(+5,10,0), 0.4f );
			AddCube( PblVector3(-5,10,0), 0.4f );

			for ( int i = 0; i < MaxCubes - 2; ++i )
			{
				float x = random_float( -Boundary, +Boundary );
				float y = random_float( 1.0f, 4.0f );
				float z = random_float( -Boundary, +Boundary );
				float scale = random_float( 0.15f, 0.15f );

				AddCube( PblVector3(x,y,z), scale );
			}			
			
		#endif
		
		#if DEMO == DEMO_REWIND_AND_REPLAY
		
			// rewind and replay
			
			// ...
			
		#endif
		
		#if DEMO == DEMO_INTERPOLATION
		
			// interpolation
			
			AddCube( PblVector3(-3,2,0), 1 );
			AddCube( PblVector3(+3,2,0), 1 );
		
		#endif
		
		// players have not joined yet
		
		for ( int i = 0; i < MaxPlayers; ++i )
			playerData[i].exists = false;
	}

	~PhysicsSimulation()
	{
		for ( int i = 0; i < numCubes; ++i )
			if ( cubes[i].fixedJoint )
				dJointDestroy( cubes[i].fixedJoint );
		
		dJointGroupDestroy( contacts );
		dSpaceDestroy( space );
		dWorldDestroy( world );
	}
	
	void Update( float deltaTime )
	{			
		// clear player data
		
		for ( int i = 0; i < MaxPlayers; ++i )
			playerData[i].attachLastFrame = false;
		
		// process player input
		
		for ( int i = 0; i < MaxPlayers; ++i )
		{
			PlayerData & player = playerData[i];
			
			if ( player.exists )
			{
				// apply strafe forces
				
				if ( player.input.left || player.input.right || player.input.forward || player.input.back )
				{
					ASSERT( player.cubeId >= 0 );
					ASSERT( player.cubeId < MaxCubes );
				
					dBodyID body = cubes[player.cubeId].body;

					const dReal * linearVelocity = dBodyGetLinearVel( body );

					PblVector3 velocity = PblVector3( linearVelocity[0], linearVelocity[1], linearVelocity[2] );
					
					float fx = 0.0f;
					float fz = 0.0f;
					
					if ( velocity.GetX() < PlayerMaximumStrafeSpeed )
						fx += PlayerStrafeForce * player.input.right;
					
					if ( velocity.GetX() > -PlayerMaximumStrafeSpeed )
						fx -= PlayerStrafeForce * player.input.left;
					
					if ( velocity.GetZ() < PlayerMaximumStrafeSpeed )
						fz += PlayerStrafeForce * player.input.forward;
					
					if ( velocity.GetZ() > -PlayerMaximumStrafeSpeed )
						fz -= PlayerStrafeForce * player.input.back;
				
					dBodyAddForce( body, fx, 0.0f, fz );

					dBodyEnable( body );
				}
			}
		}

		// detect collisions
		        
		for ( int i = 0; i < numCubes; ++i )
			cubeContacts[i] = SimulationCubeContacts();
			
		dJointGroupEmpty( contacts );
		
		dSpaceCollide( space, this, NearCallback );

		DeterminePlayerInteractions();

		#if DEMO == DEMO_KATAMARI
		
			// katamari!
			
			for ( int i = 0; i < MaxPlayers; ++i )
			{
				int playerCubeId = playerData[i].cubeId;
				
				for ( int j = 0; j < numCubes; ++j )
				{
					bool ignore = false;
					
					// dont pick up player cubes!
					for ( int k = 0; k < MaxPlayers; ++k )
					{
						if ( j == playerData[k].cubeId )
						{
							ignore = true;
						}
					}
							
					// dont pick up cubes already attached!
					if ( cubes[j].parentCubeId != -1 )
						ignore = true;
						
					if ( ignore )
						continue;
						
					if ( playerInteractions[playerCubeId].interacting[j] )
					{
						AttachCube( j, playerCubeId );
						playerData[i].attachLastFrame = true;
					}
				}
			}
			
		#endif
	
		// step world forward

		#ifdef USE_QUICK_STEP
		dWorldQuickStep( world, deltaTime );
		#else
		dWorldStep( world, deltaTime );
		#endif
	}
	
	void GetSoundState( SoundState & soundState )
	{
		for ( int i = 0; i < MaxPlayers; ++i )
		{
			if ( playerData[i].exists )
				soundState.playerAttach[i] = playerData[i].attachLastFrame;
			else
				soundState.playerAttach[i] = false;
		}
	}
	
	void GetRenderState( RenderState & renderState )
	{
		renderState.localPlayerId = -1;		// note: this is filled in by the network game
		
		for ( int i = 0; i < MaxPlayers; ++i )
		{
			renderState.player[i].exists = playerData[i].exists;
			
			if ( playerData[i].exists )
			{
				const dReal * position = dBodyGetPosition( cubes[playerData[i].cubeId].body );
				renderState.player[i].position = PblVector3( position[0], position[1], position[2] );
			}
		}
		
		ASSERT( numCubes <= MaxCubes );
		
		renderState.numCubes = numCubes;

		for ( int i = 0; i < numCubes; ++i )
		{
			ASSERT( cubes[i].body );
			
			const dReal * position = dBodyGetPosition( cubes[i].body );
			const dReal * orientation = dBodyGetQuaternion( cubes[i].body );
		
			renderState.cubes[i].position = PblVector3( position[0], position[1], position[2] );
			renderState.cubes[i].orientation = PblQuaternion( orientation[0], orientation[1], orientation[2], orientation[3] );
			renderState.cubes[i].scale = cubes[i].scale * CubeSize * 0.5f;

			ASSERT( MaxPlayers == 2 );

			bool interacting[2] = { false, false };
			
			for ( int j = 0; j < MaxPlayers; ++j )
			{
				if ( playerInteractions[j].interacting[i] )
					interacting[j] = true;
			}

			if ( playerData[0].cubeId == i )
			{
				renderState.cubes[i].r = 1.0f;
				renderState.cubes[i].g = 0.0f;
				renderState.cubes[i].b = 0.0f;
			}
			else if ( playerData[1].cubeId == i )
			{
				renderState.cubes[i].r = 0.0f;
				renderState.cubes[i].g = 0.0f;
				renderState.cubes[i].b = 1.0f;
			}
			else
			{	
				renderState.cubes[i].r = 1.0f;
				renderState.cubes[i].g = 1.0f;
				renderState.cubes[i].b = 1.0f;
			}
		}
	}
		
	void GetSimulationState( SimulationState & simulationState )
	{
		for ( int i = 0; i < MaxPlayers; ++i )
		{
			if ( playerData[i].exists )
			{
				simulationState.playerExists[i] = true;
				simulationState.playerInput[i] = playerData[i].input;
				simulationState.playerState[i].cubeId = playerData[i].cubeId;
			}
			else
			{
				simulationState.playerExists[i] = false;
			}
		}
		
		simulationState.numCubes = numCubes;
		
		for ( int i = 0; i < numCubes; ++i )
		{
			ASSERT( cubes[i].body );

			const dReal * position = dBodyGetPosition( cubes[i].body );
			const dReal * orientation = dBodyGetQuaternion( cubes[i].body );
			const dReal * linearVelocity = dBodyGetLinearVel( cubes[i].body );
			const dReal * angularVelocity = dBodyGetAngularVel( cubes[i].body );

			simulationState.cubeState[i].enabled = dBodyIsEnabled( cubes[i].body ) != 0;
			simulationState.cubeState[i].position = PblVector3( position[0], position[1], position[2] );
			simulationState.cubeState[i].orientation = PblQuaternion( orientation[0], orientation[1], orientation[2], orientation[3] );
			simulationState.cubeState[i].linearVelocity = PblVector3( linearVelocity[0], linearVelocity[1], linearVelocity[2] );
			simulationState.cubeState[i].angularVelocity = PblVector3( angularVelocity[0], angularVelocity[1], angularVelocity[2] );
		}

		memcpy( &simulationState.cubeContacts, &cubeContacts, sizeof( SimulationCubeContacts ) * MaxCubes );
		memcpy( &simulationState.playerInteractions, &playerInteractions, sizeof( SimulationPlayerInteractions ) * MaxPlayers );
	}
	
	void SetSimulationState( SimulationState & simulationState )
	{
		for ( int i = 0; i < MaxPlayers; ++i )
		{
			if ( playerData[i].exists && simulationState.playerExists[i] )
				playerData[i].input = simulationState.playerInput[i];
		}
		
		ASSERT( simulationState.numCubes == numCubes );

		for ( int i = 0; i < numCubes; ++i )
		{
			const SimulationCubeState & cubeState = simulationState.cubeState[i];

			dQuaternion quaternion;
			quaternion[0] = cubeState.orientation.GetS();
			quaternion[1] = cubeState.orientation.GetX();
			quaternion[2] = cubeState.orientation.GetY();
			quaternion[3] = cubeState.orientation.GetZ();

			dBodySetPosition( cubes[i].body, cubeState.position.GetX(), cubeState.position.GetY(), cubeState.position.GetZ() );
			dBodySetQuaternion( cubes[i].body, quaternion );
			dBodySetLinearVel( cubes[i].body, cubeState.linearVelocity.GetX(), cubeState.linearVelocity.GetY(), cubeState.linearVelocity.GetZ() );
			dBodySetAngularVel( cubes[i].body, cubeState.angularVelocity.GetX(), cubeState.angularVelocity.GetY(), cubeState.angularVelocity.GetZ() );

			dBodySetForce( cubes[i].body, 0, 0, 0 );
			dBodySetTorque( cubes[i].body, 0, 0, 0 );

			if ( cubeState.enabled )
				dBodyEnable( cubes[i].body );
			else
				dBodyDisable( cubes[i].body );
		}
	}

	void OnPlayerJoin( int playerId )
	{
		ASSERT( playerId >= 0 );
		ASSERT( playerId < MaxPlayers );

		ASSERT( playerData[playerId].exists == false );
		
		printf( "simulation: player %d joined\n", playerId );

		PlayerPossessCube( playerId, playerId );
		
		playerData[playerId].exists = true;
	}
	
	void OnPlayerLeft( int playerId )
	{
		ASSERT( playerId >= 0 );
		ASSERT( playerId < MaxPlayers );
		
		ASSERT( playerData[playerId].exists == true );
		
		printf( "simulation: player %d left\n", playerId );
		
		if ( playerData[playerId].cubeId >= 0 )
			PlayerUnpossessCube( playerId, playerData[playerId].cubeId );
			
		playerData[playerId] = PlayerData();
	}
	
protected:

	void AddCube( const PblVector3 & position, float scale )
	{
		ASSERT( numCubes >= 0 );
		ASSERT( numCubes < MaxCubes );
		
		// setup cube body
		
		cubes[numCubes].body = dBodyCreate( world );
		
		ASSERT( cubes[numCubes].body );

		dMass mass;
		dMassSetBox( &mass, 1, 1, 1, 1 );
		dMassAdjust( &mass, CubeMass * scale );
		dBodySetMass( cubes[numCubes].body, &mass );

		dBodySetPosition( cubes[numCubes].body, position.GetX(), position.GetY(), position.GetZ() );

		dBodySetData( cubes[numCubes].body, &cubes[numCubes] );

		// setup cube geom and attach to body

		cubes[numCubes].scale = scale;
		cubes[numCubes].geom = dCreateBox( space, scale * CubeSize, scale * CubeSize, scale * CubeSize );
		dGeomSetBody( cubes[numCubes].geom, cubes[numCubes].body );	

		numCubes++;
	}
	
	void PlayerPossessCube( int playerId, int cubeId )
	{
		ASSERT( playerId >= 0 );
		ASSERT( playerId < MaxPlayers );
		ASSERT( cubeId >= 0 );
		ASSERT( cubeId < MaxCubes );
		
		playerData[playerId].cubeId = cubeId;
		cubes[cubeId].ownerPlayerId = playerId;
		
		if ( cubes[cubeId].parentCubeId != -1 )
			DetachCube( cubeId );

		ASSERT( cubes[cubeId].body );
			
		dMass mass;
		dMassSetBox( &mass, 1, 1, 1, 1 );
		dMassAdjust( &mass, PlayerMass * cubes[cubeId].scale );
		dBodySetMass( cubes[cubeId].body, &mass );
	}
	
	void PlayerUnpossessCube( int playerId, int cubeId )
	{
		ASSERT( playerId >= 0 );
		ASSERT( playerId < MaxPlayers );
		ASSERT( cubeId >= 0 );
		ASSERT( cubeId < MaxCubes );
		
		cubes[cubeId].ownerPlayerId = -1;
		playerData[playerId].cubeId = -1;
		
		dMass mass;
		dMassSetBox( &mass, 1, 1, 1, 1 );
		dMassAdjust( &mass, CubeMass * cubes[cubeId].scale );
		dBodySetMass( cubes[cubeId].body, &mass );
	}

	void DeterminePlayerInteractions()
	{
		for ( int i = 0; i < MaxPlayers; ++i )
		{
			playerInteractions[i] = SimulationPlayerInteractions();
			if ( playerData[i].exists )
			{
				int cubeId = playerData[i].cubeId;
				RecurseInteractions( playerInteractions[i], cubeId );
			}
		}
	}
	
 	void RecurseInteractions( SimulationPlayerInteractions & playerInteractions, int cubeId )
	{
		ASSERT( cubeId >= 0 );
		ASSERT( cubeId < MaxCubes );
		
		for ( int i = 0; i < numCubes; ++i )
		{
			if ( cubeContacts[cubeId].touching[i] && !playerInteractions.interacting[i] )
			{
				playerInteractions.interacting[i] = true;
				RecurseInteractions( playerInteractions, i );
			}
		}		
	}
	
	void AttachCube( int childCubeId, int parentCubeId )
	{
		ASSERT( childCubeId >= 0 );
		ASSERT( childCubeId < numCubes );

		ASSERT( parentCubeId >= 0 );
		ASSERT( parentCubeId < numCubes );

		ASSERT( childCubeId != parentCubeId );

		ASSERT( cubes[childCubeId].parentCubeId == -1 );
		
		ASSERT( cubes[childCubeId].body );

		// attach via fixed joint
		
		cubes[childCubeId].fixedJoint = dJointCreateFixed( world, 0 );
		dJointAttach( cubes[childCubeId].fixedJoint, cubes[childCubeId].body, cubes[parentCubeId].body );
		dJointSetFixed( cubes[childCubeId].fixedJoint );
		
		// set parent cube id
		
		cubes[childCubeId].parentCubeId = parentCubeId;
	}
	
	void DetachCube( int childCubeId )
	{
		ASSERT( childCubeId >= 0 );
		ASSERT( childCubeId < numCubes );
		
		ASSERT( cubes[childCubeId].parentCubeId != -1 );
		
		ASSERT( cubes[childCubeId].fixedJoint );
	
		dJointDestroy( cubes[childCubeId].fixedJoint );
		cubes[childCubeId].fixedJoint = 0;
		cubes[childCubeId].parentCubeId = -1;
	}
	
	void DetachAllCubesFromParent( int parentCubeId )
	{
		ASSERT( parentCubeId >= 0 );
		ASSERT( parentCubeId < numCubes );
		
		for ( int i = 0; i < numCubes; ++i )
		{
			if ( cubes[i].parentCubeId == parentCubeId )
				DetachCube( i );
		}
	}

private:
	
	dWorldID world;
	dSpaceID space;
	dJointGroupID contacts;
	
	struct PlayerData
	{
		bool exists;
		int cubeId;
		SimulationPlayerInput input;
		bool attachLastFrame;
		
		PlayerData()
		{
			exists = false;
			cubeId = 0;
			attachLastFrame = false;
		}
	};

	PlayerData playerData[MaxPlayers];

	struct CubeData
	{
		dBodyID body;
		dGeomID geom;
		float scale;
		int parentCubeId;
		dJointID fixedJoint;
		int ownerPlayerId;
		
		CubeData()
		{
			body = 0;
			geom = 0;
			scale = 1.0f;
			parentCubeId = -1;
			fixedJoint = 0;
			ownerPlayerId = -1;
		}
	};
	
	int numCubes;
	CubeData cubes[MaxCubes];
	SimulationCubeContacts cubeContacts[MaxCubes];
	SimulationPlayerInteractions playerInteractions[MaxPlayers];

protected:
		
	static void NearCallback( void * data, dGeomID o1, dGeomID o2 )
	{
		PhysicsSimulation * simulation = (PhysicsSimulation*) data;
	
		ASSERT( simulation );
	
	    dBodyID b1 = dGeomGetBody( o1 );
	    dBodyID b2 = dGeomGetBody( o2 );

		const int MaxContacts = 4;

	    dContact contact[MaxContacts];			
	    for ( int i = 0; i < MaxContacts; i++ ) 
	    {
			contact[i].surface.mode = 0;
			contact[i].surface.mu = Friction;
	    }

	    if ( int numc = dCollide( o1, o2, MaxContacts, &contact[0].geom, sizeof(dContact) ) )
	    {
	        for ( int i = 0; i < numc; i++ ) 
	        {
	            dJointID c = dJointCreateContact( simulation->world, simulation->contacts, contact+i );
	            dJointAttach( c, b1, b2 );
	        }

			if ( b1 && b2 )
			{
				const CubeData * cube1 = (const CubeData*) dBodyGetData( b1 );
				const CubeData * cube2 = (const CubeData*) dBodyGetData( b2 );
			
				ASSERT( cube1 );
				ASSERT( cube2 );
			
				if ( cube1->parentCubeId == -1 && cube2->parentCubeId == -1 )
				{
					int cubeId1 = (int) ( uint64(cube1) - uint64(simulation->cubes) ) / sizeof( CubeData );
					int cubeId2 = (int) ( uint64(cube2) - uint64(simulation->cubes) ) / sizeof( CubeData );
			
					if ( ! ( cube1->ownerPlayerId != -1 && cube1->ownerPlayerId == cube2->parentCubeId ||
						     cube2->ownerPlayerId != -1 && cube2->ownerPlayerId == cube1->parentCubeId ) )
					{
						ASSERT( cubeId1 >= 0 );
						ASSERT( cubeId2 >= 0 );
						ASSERT( cubeId1 < MaxCubes );
						ASSERT( cubeId2 < MaxCubes );
	
						simulation->cubeContacts[cubeId1].touching[cubeId2] = true;
						simulation->cubeContacts[cubeId2].touching[cubeId1] = true;
					}
				}
			}
	    }
	}
};
*/

// ------------------------------------------------------------------------------

int main( int argc, char * argv[] )
{
	// ...

	return 0;
}

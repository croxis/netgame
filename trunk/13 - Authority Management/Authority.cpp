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
#include "Mathematics.h"
//#include "Transport.h"

using namespace std;
using namespace net;

// ------------------------------------------------------------------------------

// constants

const int DisplayWidth = 800;
const int DisplayHeight = 600;

const float ShadowDistance = 10.0f;
const float ColorChangeTightness = 0.1f;

const float AuthorityTimeout = 1.0f;
const float AuthorityLinearRestThreshold = 0.1f;
const float AuthorityAngularRestThreshold = 0.1f;

const int MaxCubes = 64;
const int MaxPlayers = 2;
const int PlayerBits = 1;
const float CubeSize = 2.0f;
const float CubeMass = 0.1f;
const float PlayerStrafeForce = 30.0f;
const float PlayerMaximumStrafeSpeed = 10.0f;
const float PlayerMass = 1.0f;
const float Boundary = 10.0f;
const float Gravity = 20.0f;
const float Friction = 30.0f;
const int MaxIterations = 20;
const float ERP = 0.2f;
const float CFM = 0.05f;
const float MaximumCorrectingVelocity = 100.0f;
const float ContactSurfaceLayer = 0.001f;

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
		orientation = math::Quaternion(1,0,0,0);
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
};

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

		srand( 280090412 );

		AddCube( math::Vector(-5,10,0), 1 );
		AddCube( math::Vector(+5,10,0), 1 );

		AddCube( math::Vector(0,1,0), 0.9f );
		AddCube( math::Vector(0,4,0), 0.9f );
		AddCube( math::Vector(0,8,0), 0.9f );

		for ( int i = 0; i < 30; ++i )
		{
			float x = math::random_float( -Boundary, +Boundary );
			float y = math::random_float( 1.0f, 4.0f );
			float z = math::random_float( -Boundary, +Boundary );
			float scale = math::random_float( 0.1f, 0.6f );

			AddCube( math::Vector(x,y,z), scale );
		}
		
		srand( 100 );

		for ( int i = 0; i < 29; ++i )
		{
			float x = math::random_float( -Boundary, +Boundary );
			float y = math::random_float( 2.0f, 4.0f );
			float z = math::random_float( -Boundary, +Boundary );
			float scale = math::random_float( 0.1f, 0.2f );

			AddCube( math::Vector(x,y,z), scale );
		}
			
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
		// process player input
		
		for ( int i = 0; i < MaxPlayers; ++i )
		{
			PlayerData & player = playerData[i];
			
			if ( player.exists )
			{
				// apply strafe forces
				
				if ( player.input.left || player.input.right || player.input.forward || player.input.back )
				{
					assert( player.cubeId >= 0 );
					assert( player.cubeId < MaxCubes );
				
					dBodyID body = cubes[player.cubeId].body;

					const dReal * linearVelocity = dBodyGetLinearVel( body );

					math::Vector velocity( linearVelocity[0], linearVelocity[1], linearVelocity[2] );
					
					float fx = 0.0f;
					float fz = 0.0f;
					
					if ( velocity.x < PlayerMaximumStrafeSpeed )
						fx += PlayerStrafeForce * player.input.right;
					
					if ( velocity.x > -PlayerMaximumStrafeSpeed )
						fx -= PlayerStrafeForce * player.input.left;
					
					if ( velocity.z < PlayerMaximumStrafeSpeed )
						fz += PlayerStrafeForce * player.input.forward;
					
					if ( velocity.z > -PlayerMaximumStrafeSpeed )
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
	
		// step world forward

		#ifdef USE_QUICK_STEP
		dWorldQuickStep( world, deltaTime );
		#else
		dWorldStep( world, deltaTime );
		#endif
	}
		
	/*
	void GetRenderState( RenderState & renderState )
	{
		renderState.localPlayerId = -1;		// note: this is filled in by the network game
		
		for ( int i = 0; i < MaxPlayers; ++i )
		{
			renderState.player[i].exists = playerData[i].exists;
			
			if ( playerData[i].exists )
			{
				const dReal * position = dBodyGetPosition( cubes[playerData[i].cubeId].body );
				renderState.player[i].position = math::Vector( position[0], position[1], position[2] );
			}
		}
		
		assert( numCubes <= MaxCubes );
		
		renderState.numCubes = numCubes;

		for ( int i = 0; i < numCubes; ++i )
		{
			assert( cubes[i].body );
			
			const dReal * position = dBodyGetPosition( cubes[i].body );
			const dReal * orientation = dBodyGetQuaternion( cubes[i].body );
		
			renderState.cubes[i].position = math::Vector( position[0], position[1], position[2] );
			renderState.cubes[i].orientation = math::Quaternion( orientation[0], orientation[1], orientation[2], orientation[3] );
			renderState.cubes[i].scale = cubes[i].scale * CubeSize * 0.5f;

			assert( MaxPlayers == 2 );

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
	*/
		
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
			assert( cubes[i].body );

			const dReal * position = dBodyGetPosition( cubes[i].body );
			const dReal * orientation = dBodyGetQuaternion( cubes[i].body );
			const dReal * linearVelocity = dBodyGetLinearVel( cubes[i].body );
			const dReal * angularVelocity = dBodyGetAngularVel( cubes[i].body );

			simulationState.cubeState[i].enabled = dBodyIsEnabled( cubes[i].body ) != 0;
			simulationState.cubeState[i].position = math::Vector( position[0], position[1], position[2] );
			simulationState.cubeState[i].orientation = math::Quaternion( orientation[0], orientation[1], orientation[2], orientation[3] );
			simulationState.cubeState[i].linearVelocity = math::Vector( linearVelocity[0], linearVelocity[1], linearVelocity[2] );
			simulationState.cubeState[i].angularVelocity = math::Vector( angularVelocity[0], angularVelocity[1], angularVelocity[2] );
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
		
		assert( simulationState.numCubes == numCubes );

		for ( int i = 0; i < numCubes; ++i )
		{
			const SimulationCubeState & cubeState = simulationState.cubeState[i];

			dQuaternion quaternion;
			quaternion[0] = cubeState.orientation.w;
			quaternion[1] = cubeState.orientation.x;
			quaternion[2] = cubeState.orientation.y;
			quaternion[3] = cubeState.orientation.z;

			dBodySetPosition( cubes[i].body, cubeState.position.x, cubeState.position.y, cubeState.position.z );
			dBodySetQuaternion( cubes[i].body, quaternion );
			dBodySetLinearVel( cubes[i].body, cubeState.linearVelocity.x, cubeState.linearVelocity.y, cubeState.linearVelocity.z );
			dBodySetAngularVel( cubes[i].body, cubeState.angularVelocity.x, cubeState.angularVelocity.y, cubeState.angularVelocity.z );

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
		assert( playerId >= 0 );
		assert( playerId < MaxPlayers );

		assert( playerData[playerId].exists == false );
		
		printf( "simulation: player %d joined\n", playerId );

		PlayerPossessCube( playerId, playerId );
		
		playerData[playerId].exists = true;
	}
	
	void OnPlayerLeft( int playerId )
	{
		assert( playerId >= 0 );
		assert( playerId < MaxPlayers );
		
		assert( playerData[playerId].exists == true );
		
		printf( "simulation: player %d left\n", playerId );
		
		if ( playerData[playerId].cubeId >= 0 )
			PlayerUnpossessCube( playerId, playerData[playerId].cubeId );
			
		playerData[playerId] = PlayerData();
	}
	
protected:

	void AddCube( const math::Vector & position, float scale )
	{
		assert( numCubes >= 0 );
		assert( numCubes < MaxCubes );
		
		// setup cube body
		
		cubes[numCubes].body = dBodyCreate( world );
		
		assert( cubes[numCubes].body );

		dMass mass;
		dMassSetBox( &mass, 1, 1, 1, 1 );
		dMassAdjust( &mass, CubeMass * scale );
		dBodySetMass( cubes[numCubes].body, &mass );

		dBodySetPosition( cubes[numCubes].body, position.x, position.y, position.z );

		dBodySetData( cubes[numCubes].body, &cubes[numCubes] );

		// setup cube geom and set body

		cubes[numCubes].scale = scale;
		cubes[numCubes].geom = dCreateBox( space, scale * CubeSize, scale * CubeSize, scale * CubeSize );
		dGeomSetBody( cubes[numCubes].geom, cubes[numCubes].body );	

		numCubes++;
	}
	
	void PlayerPossessCube( int playerId, int cubeId )
	{
		assert( playerId >= 0 );
		assert( playerId < MaxPlayers );
		assert( cubeId >= 0 );
		assert( cubeId < MaxCubes );
		
		playerData[playerId].cubeId = cubeId;
		cubes[cubeId].ownerPlayerId = playerId;
		
		assert( cubes[cubeId].body );
			
		dMass mass;
		dMassSetBox( &mass, 1, 1, 1, 1 );
		dMassAdjust( &mass, PlayerMass * cubes[cubeId].scale );
		dBodySetMass( cubes[cubeId].body, &mass );
	}
	
	void PlayerUnpossessCube( int playerId, int cubeId )
	{
		assert( playerId >= 0 );
		assert( playerId < MaxPlayers );
		assert( cubeId >= 0 );
		assert( cubeId < MaxCubes );
		
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
		assert( cubeId >= 0 );
		assert( cubeId < MaxCubes );
		
		for ( int i = 0; i < numCubes; ++i )
		{
			if ( cubeContacts[cubeId].touching[i] && !playerInteractions.interacting[i] )
			{
				playerInteractions.interacting[i] = true;
				RecurseInteractions( playerInteractions, i );
			}
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
		
		PlayerData()
		{
			exists = false;
			cubeId = 0;
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
	
		assert( simulation );
	
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
			
				assert( cube1 );
				assert( cube2 );
			
				if ( cube1->parentCubeId == -1 && cube2->parentCubeId == -1 )
				{
					int cubeId1 = (int) ( uint64(cube1) - uint64(simulation->cubes) ) / sizeof( CubeData );
					int cubeId2 = (int) ( uint64(cube2) - uint64(simulation->cubes) ) / sizeof( CubeData );
			
					if ( ! ( cube1->ownerPlayerId != -1 && cube1->ownerPlayerId == cube2->parentCubeId ||
						     cube2->ownerPlayerId != -1 && cube2->ownerPlayerId == cube1->parentCubeId ) )
					{
						assert( cubeId1 >= 0 );
						assert( cubeId2 >= 0 );
						assert( cubeId1 < MaxCubes );
						assert( cubeId2 < MaxCubes );
	
						simulation->cubeContacts[cubeId1].touching[cubeId2] = true;
						simulation->cubeContacts[cubeId2].touching[cubeId1] = true;
					}
				}
			}
	    }
	}
};

// ------------------------------------------------------------------------------

int main( int argc, char * argv[] )
{
	// ...

	return 0;
}

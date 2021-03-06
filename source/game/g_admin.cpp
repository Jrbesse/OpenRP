#include "OpenRP.h"
#include "g_local.h"
#include "g_admin.h"
#include "g_character.h"

extern qboolean M_PartialMatch( const char * s1, const char * s2 );
extern qboolean M_IsInteger( const char * name );
extern int M_G_ClientNumberFromName ( const char* name );

extern void AddSkill(gentity_t *self, int amount);
extern void Admin_Teleport( gentity_t *ent );

/*
==================

M_HolsterThoseSabers - MJN
Something like Cmd_ToggleSaber,
but stripped down and for holster only.
==================
*/
void M_HolsterThoseSabers( gentity_t *ent ){

        // MJN - Check to see if that is the weapon of choice...
        if (ent->client->ps.weapon != WP_SABER)
        {
                return;
        }
        // MJN - Cannot holster it in flight or we're screwed!
        if (ent->client->ps.saberInFlight)
        {
                return;
        }
        // MJN - Cannot holster in saber lock.
        if (ent->client->ps.saberLockTime >= level.time)
        {
                return;
        }
        // MJN - Holster Sabers
        if ( ent->client->ps.saberHolstered < 2 )
		{
            if (ent->client->saber[0].soundOff)
			{
				G_Sound(ent, CHAN_AUTO, ent->client->saber[0].soundOff);
			}

			if (ent->client->saber[1].soundOff && ent->client->saber[1].model[0])
			{
					G_Sound(ent, CHAN_AUTO, ent->client->saber[1].soundOff);
			}
			ent->client->ps.saberHolstered = 2;
			ent->client->ps.weaponTime = 400;
        }
}

/*
==================
M_SanitizeString

Remove case and control characters (Same as in g_cmds.c).
==================
*/
static void M_SanitizeString( char *in, char *out )
{
	int i = 0;
	int r = 0;

	while (in[i])
	{
		if (i >= MAX_NAME_LENGTH-1)
		{ //the ui truncates the name here..
			break;
		}

		if (in[i] == '^')
		{
			if (in[i+1] >= 48 && //'0'
				in[i+1] <= 57) //'9'
			{ //only skip it if there's a number after it for the color
				i += 2;
				continue;
			}
			else
			{ //just skip the ^
				i++;
				continue;
			}
		}

		if (in[i] < 32)
		{
			i++;
			continue;
		}

		out[r] = in[i];
		r++;
		i++;
	}
	out[r] = 0;
}
/*
==================
M_SanitizeString2

Remove case and control characters
==================
*/
static void M_SanitizeString2( char *in, char *out ) {
	while ( *in ) {
		if ( *in == 27 ) {
			in += 2;		// skip color code
			continue;
		}
		if ( *in < 32 ) {
			in++;
			continue;
		}
		*out++ = tolower( *in++ );
	}
	*out = 0;
}
qboolean G_CheckAdmin(gentity_t *ent, int bitvalue)
{
	int Bitvalues = 0;

	if ( !( bitvalue & ADMIN_CHEATS ) )
		CheckAdmin( ent );

	if ( !ent->client->sess.isAdmin )
		return qfalse;

	//Right they are admin so lets check what sort so we can assign bitvalues
	if(ent->client->sess.adminLevel == 1)
		Bitvalues = openrp_admin1Bitvalues.integer;
	else if(ent->client->sess.adminLevel == 2)
		Bitvalues = openrp_admin2Bitvalues.integer;
	else if(ent->client->sess.adminLevel == 3)
		Bitvalues = openrp_admin3Bitvalues.integer;
	else if(ent->client->sess.adminLevel == 4)
		Bitvalues = openrp_admin4Bitvalues.integer;
	else if(ent->client->sess.adminLevel == 5)
		Bitvalues = openrp_admin5Bitvalues.integer;
	else if(ent->client->sess.adminLevel == 6)
		Bitvalues = openrp_admin6Bitvalues.integer;
	else if(ent->client->sess.adminLevel == 7)
		Bitvalues = openrp_admin7Bitvalues.integer;
	else if(ent->client->sess.adminLevel == 8)
		Bitvalues = openrp_admin8Bitvalues.integer;
	else if(ent->client->sess.adminLevel == 9)
		Bitvalues = openrp_admin9Bitvalues.integer;
	else if(ent->client->sess.adminLevel == 10)
		Bitvalues = openrp_admin10Bitvalues.integer;

	//If bitvalues = 0, they don't have access to any admin command
	if( !Bitvalues )
		return qfalse;

	if(Bitvalues & bitvalue)
		return qtrue;
	else
		return qfalse;
}

/*
============
Admin Control - Determines whether admins can perform admin commands on higher admin levels
============
*/
qboolean G_AdminControl( gentity_t *ent, gentity_t *target)
{
	if( !openrp_adminControl.integer )
		return qtrue;

	if ( !target->client->sess.isAdmin )
		return qtrue;

	//Less than is used instead of greater than because admin level 1 is higher than admin level 2
	if( openrp_adminControl.integer  && ( ent->client->sess.adminLevel <= target->client->sess.adminLevel ) )				
		return qtrue;
	else
		return qfalse;
}

/*
============
amban Function
============
*/
void Cmd_amBan_F(gentity_t *ent)
{
	char cmdTarget[MAX_STRING_CHARS];
	int clientid = -1; 

	if(!G_CheckAdmin(ent, ADMIN_BAN))
	{  	
		trap_SendServerCommand(ent-g_entities, va("print \"^1You are not allowed to use this command.\n\""));	  	
		return;  	
	}

	if(trap_Argc() < 2)
	{
		trap_SendServerCommand(ent-g_entities, va("print \"^2Command Usage: /amban <name/clientid>\n\""));
		return;
	}

	trap_Argv(1, cmdTarget, MAX_STRING_CHARS);

	clientid = M_G_ClientNumberFromName( cmdTarget );
	if (clientid == -1) 
	{ 
		trap_SendServerCommand( ent-g_entities, va("print \"Can't find client ID for %s\n\"", cmdTarget ) ); 
		return; 
	} 
	if (clientid == -2) 
	{ 
		trap_SendServerCommand( ent-g_entities, va("print \"Ambiguous client ID for %s\n\"", cmdTarget ) ); 
		return; 
	}
	if (clientid >= MAX_CLIENTS || clientid < 0) 
	{ 
		trap_SendServerCommand( ent-g_entities, va("print \"Bad client ID for %s\n", cmdTarget ) );
		return;
	}
	if (!g_entities[clientid].inuse) 
	{
		trap_SendServerCommand( ent-g_entities, va("print \"Client %s is not active\n\"", cmdTarget ) ); 
		return; 
	}

	if(ent-g_entities == clientid)
	{
		trap_SendServerCommand(ent-g_entities, va("print \"^2You can't ban yourself.\n\""));
		return;
	}

	if( !G_AdminControl( ent, &g_entities[clientid] ) )
	{
		trap_SendServerCommand(ent-g_entities, va("print \"^1You can't use this command on them. They are a higher admin level than you.\n\""));
		return;
	}
	
	if (!(g_entities[clientid].r.svFlags & SVF_BOT))
	{
		AddIP(g_entities[clientid].client->sess.IP);
		trap_SendServerCommand(ent-g_entities, va("print \"^2The IP of the person you banned is ^7%s\n\"", g_entities[clientid].client->sess.IP));
	}
	trap_DropClient(clientid, "^1was ^1permanently ^1banned.\n");

	G_LogPrintf("Ban admin command executed by %s on %s.\n", ent->client->pers.netname, g_entities[clientid].client->pers.netname);
	return;
}

/*
============
amkick Function
============
*/
void Cmd_amKick_F(gentity_t *ent)
{
	char cmdTarget[MAX_STRING_CHARS];
	int clientid = -1;

	if(!G_CheckAdmin(ent, ADMIN_KICK))
	{  	
		trap_SendServerCommand(ent-g_entities, va("print \"^1You are not allowed to use this command.\n\""));	  	
		return;  	
	}

	if(trap_Argc() < 2)
	{
		trap_SendServerCommand(ent-g_entities, va("print \"^2Command Usage: /amkick <name/clientid>\n\""));
		return;
	}

	trap_Argv(1, cmdTarget, MAX_STRING_CHARS);

	clientid = M_G_ClientNumberFromName( cmdTarget );
	if (clientid == -1) 
	{ 
		trap_SendServerCommand( ent-g_entities, va("print \"Can't find client ID for %s\n\"", cmdTarget ) ); 
		return; 
	} 
	if (clientid == -2) 
	{ 
		trap_SendServerCommand( ent-g_entities, va("print \"Ambiguous client ID for %s\n\"", cmdTarget ) ); 
		return; 
	}
	if (clientid >= MAX_CLIENTS || clientid < 0) 
	{ 
		trap_SendServerCommand( ent-g_entities, va("print \"Bad client ID for %s\n", cmdTarget ) );
		return;
	}
	if (!g_entities[clientid].inuse) 
	{
		trap_SendServerCommand( ent-g_entities, va("print \"Client %s is not active\n\"", cmdTarget ) ); 
		return; 
	}

	if( !G_AdminControl( ent, &g_entities[clientid] ) )
	{
		trap_SendServerCommand(ent-g_entities, va("print \"^1You can't use this command on them. They are a higher admin level than you.\n\""));
		return;
	}

	trap_SendServerCommand(ent-g_entities, va("print \"^2The IP of the person you kicked is %s\n\"", g_entities[clientid].client->sess.IP));
	trap_DropClient(clientid, "^1was ^1kicked.");
	G_LogPrintf("Kick admin command executed by %s on %s.\n", ent->client->pers.netname, g_entities[clientid].client->pers.netname);
	return;
}

/*
============
amwarn Function
============
*/
void Cmd_amWarn_F(gentity_t *ent)
{
	char cmdTarget[MAX_STRING_CHARS];
	int clientid = -1;

	if(!G_CheckAdmin(ent, ADMIN_WARN))
	{  	
		trap_SendServerCommand(ent-g_entities, va("print \"^1You are not allowed to use this command.\n\""));	  	
		return;  	
	}

	if(trap_Argc() < 2)
	{
		trap_SendServerCommand(ent-g_entities, va("print \"^2Command Usage: /amwarn <name/clientid>\n\""));
		return;
	}

	trap_Argv(1, cmdTarget, MAX_STRING_CHARS);

	clientid = M_G_ClientNumberFromName( cmdTarget );
	if (clientid == -1) 
	{ 
		trap_SendServerCommand( ent-g_entities, va("print \"Can't find client ID for %s\n\"", cmdTarget ) ); 
		return; 
	} 
	if (clientid == -2) 
	{ 
		trap_SendServerCommand( ent-g_entities, va("print \"Ambiguous client ID for %s\n\"", cmdTarget ) ); 
		return; 
	}
	if (clientid >= MAX_CLIENTS || clientid < 0) 
	{ 
		trap_SendServerCommand( ent-g_entities, va("print \"Bad client ID for %s\n", cmdTarget ) );
		return;
	}
	if (!g_entities[clientid].inuse) 
	{
		trap_SendServerCommand( ent-g_entities, va("print \"Client %s is not active\n\"", cmdTarget ) ); 
		return; 
	}

	if( !G_AdminControl( ent, &g_entities[clientid] ) )
	{
		trap_SendServerCommand(ent-g_entities, va("print \"^1You can't use this command on them. They are a higher admin level than you.\n\""));
		return;
	}

	g_entities[clientid].client->sess.warnings++;

	trap_SendServerCommand( ent-g_entities, va( "print \"^2Player %s ^2was warned.\nThey have ^7%i/%i ^1warnings.\n\"", g_entities[clientid].client->pers.netname, g_entities[clientid].client->sess.warnings, openrp_maxWarnings.integer ) );
	
	trap_SendServerCommand( clientid, va( "print \"^1You have been warned by an admin.\nYou have ^7%i/%i ^1warnings.\n\"", g_entities[clientid].client->sess.warnings, openrp_maxWarnings.integer ) );
	trap_SendServerCommand( clientid, va( "cp \"^1You have been warned by an admin.\nYou have ^7%i/%i ^1warnings.\n\"", g_entities[clientid].client->sess.warnings, openrp_maxWarnings.integer ) );
	G_LogPrintf("Warn admin command executed by %s on %s.\n", ent->client->pers.netname, g_entities[clientid].client->pers.netname);

	if( g_entities[clientid].client->sess.warnings == atoi( openrp_maxWarnings.string ) )
	{
		trap_DropClient(clientid, "^1was ^1kicked ^1- ^1received ^1maximum ^1number ^1of ^1warnings ^1from ^1admins.\n");
		G_LogPrintf("%s was kicked - received maximum number of warnings from admins.\n", g_entities[clientid].client->pers.netname);
		return;
	}
	return;
}

/*
============
amtele Function
============
*/
void Cmd_amTeleport_F(gentity_t *ent)
{
	vec3_t location;
	vec3_t forward;
	vec3_t origin;
	vec3_t yaw;
	int	clientid = -1;
	int	clientid2 = -1;
	char	arg1[MAX_STRING_CHARS];
	char	arg2[MAX_STRING_CHARS];
	char	buffer[MAX_TOKEN_CHARS]; 

	if(!G_CheckAdmin(ent, ADMIN_TELEPORT))
	{  	
		trap_SendServerCommand(ent-g_entities, va("print \"^1You are not allowed to use this command.\n\""));	  	
		return;  	
	}

	if ( trap_Argc() == 1 )
	{
		//cm NOTE: This is where you teleport to a the telemark.
		if (ent->client->pers.amtelemark1 == 0 && ent->client->pers.amtelemark2 == 0 && 
				ent->client->pers.amtelemark3 == 0 && ent->client->pers.amtelemarkyaw == 0 &&
				!ent->client->pers.amtelemarkset )
		{
			trap_SendServerCommand( ent-g_entities, va("print \"^1You do not have a telemark set.\nUse /amtelemark to establish a telemark.\n\"") );
			return;
		} 
		else 
		{
			origin[0] = ent->client->pers.amtelemark1;
			origin[1] = ent->client->pers.amtelemark2;
			origin[2] = ent->client->pers.amtelemark3;
			yaw[0] = 0.0f;
			yaw[1] = ent->client->pers.amtelemarkyaw;
			yaw[2] = 0.0f;
			TeleportPlayer( ent, origin, yaw );
		}
	}
	//cm - Dom
	//Teleport to player
	if ( trap_Argc() == 2 )
	{
		trap_Argv( 1, arg1, sizeof( arg1 ) );
		clientid = M_G_ClientNumberFromName( arg1 );

		if (clientid == -1)
		{
			trap_SendServerCommand( ent-g_entities, va("print \"Can't find client ID for %s\n\"", arg1 ) );
			return;
		}
		if (clientid == -2)
		{
			trap_SendServerCommand( ent-g_entities, va("print \"Ambiguous client ID for %s\n\"", arg1 ) );
			return;
		}
		if (clientid >= MAX_CLIENTS || clientid < 0) 
		{ 
			trap_SendServerCommand( ent-g_entities, va("print \"Bad client ID for %s\n", arg1 ) );
			return;
		}
		// either we have the client id or the string did not match
		if (!g_entities[clientid].inuse)
		{ // check to make sure client slot is in use
			trap_SendServerCommand( ent-g_entities, va("print \"Client %s is not active\n\"", arg1 ) );
			return;
		}
		if (g_entities[clientid].health <= 0)
		{
			return;
		}
		if ( clientid == ent-g_entities )
		{
			trap_SendServerCommand( ent-g_entities, va("print \"^1You can't teleport yourself.\n\""));
			return;
		}
		//Copy their location
		VectorCopy(g_entities[clientid].client->ps.origin, location);
		AngleVectors(ent->client->ps.viewangles, forward, NULL, NULL);
		// set location out in front of your view
		forward[2] = 0; //no elevation change
		VectorNormalize(forward);
		VectorMA(g_entities[clientid].client->ps.origin, 100, forward, location);
		location[2] += 5; //add just a bit of height???
		//Teleport you to them
		TeleportPlayer(ent, location, g_entities[clientid].client->ps.viewangles);
		G_LogPrintf("Teleport admin command is executed by %s on %s.\n", ent->client->pers.netname, g_entities[clientid].client->pers.netname);
	}
	//Teleport player to player
	if ( trap_Argc() == 3 )
	{
		trap_Argv( 1, arg1, sizeof( arg1 ) );
		trap_Argv( 2, arg2, sizeof( arg2 ) );
		clientid = M_G_ClientNumberFromName( arg1 );
		clientid2 = M_G_ClientNumberFromName( arg2 );

		if (clientid == -1)
		{
			trap_SendServerCommand( ent-g_entities, va("print \"Can't find client ID for %s\n\"", arg1 ) );
			return;
		}
		if (clientid == -2)
		{
			trap_SendServerCommand( ent-g_entities, va("print \"Ambiguous client ID for %s\n\"", arg1 ) );
			return;
		}

		if (clientid2 == -1)
		{
			trap_SendServerCommand( ent-g_entities, va("print \"Can't find client ID for %s\n\"", arg2 ) );
			return;
		}
		if (clientid2 == -2)
		{
			trap_SendServerCommand( ent-g_entities, va("print \"Ambiguous client ID for %s\n\"", arg2 ) );
			return;
		}
		if (clientid >= MAX_CLIENTS || clientid < 0) 
		{ 
			trap_SendServerCommand( ent-g_entities, va("print \"Bad client ID for %s\n", arg1 ) );
			return;
		}
		if (clientid2 >= MAX_CLIENTS || clientid2 < 0) 
		{ 
			trap_SendServerCommand( ent-g_entities, va("print \"Bad client ID for %s\n", arg1 ) );
			return;
		}

		// either we have the client id or the string did not match
		if (!g_entities[clientid].inuse)
		{ // check to make sure client slot is in use
			trap_SendServerCommand( ent-g_entities, va("print \"Client %s is not active\n\"", arg1 ) );
			return;
		}
		if (g_entities[clientid].health <= 0)
		 {
			return;
		 }

		// either we have the client id or the string did not match
		if (!g_entities[clientid2].inuse)
		{ // check to make sure client slot is in use
			trap_SendServerCommand( ent-g_entities, va("print \"Client %s is not active\n\"", arg2 ) );
			return;
		}
		if (g_entities[clientid2].health <= 0)
		 {
			return;
		 }
		if ( clientid == clientid2 )
		{
			trap_SendServerCommand( ent-g_entities, va("print \"Cant teleport client to same client.\n\""));
			return;
		}

		if( !G_AdminControl( ent, &g_entities[clientid] ) )
		{
			trap_SendServerCommand(ent-g_entities, va("print \"^1You can't use this command on them. They are a higher admin level than you.\n\""));
			return;
		}

		//Copy client 2 origin
		VectorCopy(g_entities[clientid2].client->ps.origin, location);
		AngleVectors(g_entities[clientid2].client->ps.viewangles, forward, NULL, NULL);
		// set location out in front of your view
		forward[2] = 0; //no elevation change
		VectorNormalize(forward);
		VectorMA(g_entities[clientid2].client->ps.origin, 100, forward, location);
		location[2] += 5; //add just a bit of height???
		//Teleport you to them
		TeleportPlayer(&g_entities[clientid], location, g_entities[clientid2].client->ps.viewangles);
		G_LogPrintf("Teleport admin command is executed by %s on %s.\n", ent->client->pers.netname, g_entities[clientid].client->pers.netname);
	}
	//Using manual coordinates
	if ( trap_Argc() == 4 )
	{
			Admin_Teleport(ent);
	}
	//cm - Dom
	//Teleport player to manual coordinates
	if ( trap_Argc() == 5 )
	{	
		trap_Argv( 1, arg1, sizeof( arg1 ) );
		
		clientid = M_G_ClientNumberFromName( arg1 );
			

		if (clientid == -1)
		{
			trap_SendServerCommand( ent-g_entities, va("print \"Can't find client ID for %s\n\"", arg1 ) );
			return;
		}
		if (clientid == -2)
		{
			trap_SendServerCommand( ent-g_entities, va("print \"Ambiguous client ID for %s\n\"", arg1 ) );
			return;
		}
		if (clientid >= MAX_CLIENTS || clientid < 0) 
		{ 
			trap_SendServerCommand( ent-g_entities, va("print \"Bad client ID for %s\n", arg1 ) );
			return;
		}
		// either we have the client id or the string did not match
		if (!g_entities[clientid].inuse)
		{ // check to make sure client slot is in use
			trap_SendServerCommand( ent-g_entities, va("print \"Client %s is not active\n\"", arg1 ) );
			return;
		}
		if (g_entities[clientid].health <= 0)
		{
			return;
		}

		//Taken from Admin_Teleport() with some mods			
		trap_Argv(2, buffer, sizeof( buffer ) );
		origin[0] = atof(buffer);
		trap_Argv(3, buffer, sizeof( buffer ) );
		origin[1] = atof(buffer);
		trap_Argv(4, buffer, sizeof( buffer ) );
		origin[2] = atof(buffer);			

		TeleportPlayer( &g_entities[clientid], origin, g_entities[clientid].client->ps.viewangles );
		G_LogPrintf("Teleport admin command is executed by %s on %s.\n", ent->client->pers.netname, g_entities[clientid].client->pers.netname);
		return;
	}
	return;
}

/*
============
amannounce Function
============
*/
void Cmd_amAnnounce_F(gentity_t *ent)
{ 
	int pos = 0;
	char real_msg[MAX_STRING_CHARS];
	char *msg = ConcatArgs(2);
	char cmdTarget[MAX_STRING_CHARS];
	int clientid = -1;
	int i;

	if(!G_CheckAdmin(ent, ADMIN_ANNOUNCE))
	{  	
		trap_SendServerCommand(ent-g_entities, va("print \"^1You are not allowed to use this command.\n\""));	  	
		return;  	
	}

	while(*msg)
	{ 
		if(msg[0] == '\\' && msg[1] == 'n')
		{ 
			msg++;
			real_msg[pos++] = '\n';
		} 
		else
		{
			real_msg[pos++] = *msg;
		} 
		msg++;
	}

	real_msg[pos] = 0;

	if ( trap_Argc() < 2 )
	{ 
		trap_SendServerCommand( ent-g_entities, va ( "print \"^2Command Usage: /amannounce <name/clientid> <message>\nUse all or -1 for the clientid if you want to announce something to all players.\n\"" ) ); 
		return;
	}

	trap_Argv(1, cmdTarget, MAX_STRING_CHARS);

	if(!Q_stricmp(cmdTarget, "all") || (!Q_stricmp(cmdTarget, "-1") ))
	{
		for( i = 0; i < level.maxclients; i++ )
		{
			if( g_entities[i].inuse && g_entities[i].client && g_entities[i].client->pers.connected == CON_CONNECTED )
			{
				G_Sound( &g_entities[i], CHAN_MUSIC, G_SoundIndex( "sound/OpenRP/info.mp3" ) );
			}
		}
		trap_SendServerCommand( -1, va("print \"%s\n\"", real_msg) );
		trap_SendServerCommand( -1, va("cp \"%s\"", real_msg) );
		G_LogPrintf("Announce admin command executed by %s. The announcement was: %s\n", ent->client->pers.netname, real_msg);
		return;
	}

	clientid = M_G_ClientNumberFromName( cmdTarget );
	if (clientid == -1) 
	{ 
		trap_SendServerCommand( ent-g_entities, va("print \"Can't find client ID for %s\n\"", cmdTarget ) ); 
		return; 
	} 
	if (clientid == -2) 
	{ 
		trap_SendServerCommand( ent-g_entities, va("print \"Ambiguous client ID for %s\n\"", cmdTarget ) ); 
		return; 
	}
	if (clientid >= MAX_CLIENTS || clientid < 0) 
	{ 
		trap_SendServerCommand( ent-g_entities, va("print \"Bad client ID for %s\n", cmdTarget ) );
		return;
	}
	if (!g_entities[clientid].inuse) 
	{
		trap_SendServerCommand( ent-g_entities, va("print \"Client %s is not active\n\"", cmdTarget ) ); 
		return; 
	}
	trap_SendServerCommand( clientid, "localSound sound/OpenRP/info.mp3" );
	trap_SendServerCommand(clientid, va("print \"%s\n\"", real_msg));
	trap_SendServerCommand(clientid, va("cp \"%s\"", real_msg));
	G_LogPrintf("Announce admin command executed by %s. It was sent to %s. The announcement was: %s\n", ent->client->pers.netname, g_entities[clientid].client->pers.netname, real_msg);
	return;
}

/*
============
amsilence Function
============
*/
void Cmd_amSilence_F(gentity_t *ent)
{
	char cmdTarget[MAX_STRING_CHARS];
	int clientid = -1;

	if(!G_CheckAdmin(ent, ADMIN_SILENCE))
	{  	
		trap_SendServerCommand(ent-g_entities, va("print \"^1You are not allowed to use this command.\n\""));	  	
		return;  	
	}

	if(trap_Argc() < 2)
	{
		trap_SendServerCommand(ent-g_entities, va("print \"^2Command Usage: /amsilence <name/clientid>\n\""));
		return;
	}

	trap_Argv(1, cmdTarget, MAX_STRING_CHARS);

	clientid = M_G_ClientNumberFromName( cmdTarget );
	if (clientid == -1) 
	{ 
		trap_SendServerCommand( ent-g_entities, va("print \"Can't find client ID for %s\n\"", cmdTarget ) ); 
		return; 
	} 
	if (clientid == -2) 
	{ 
		trap_SendServerCommand( ent-g_entities, va("print \"Ambiguous client ID for %s\n\"", cmdTarget ) ); 
		return; 
	}
	if (clientid >= MAX_CLIENTS || clientid < 0) 
	{ 
		trap_SendServerCommand( ent-g_entities, va("print \"Bad client ID for %s\n", cmdTarget ) );
		return;
	}
	if (!g_entities[clientid].inuse) 
	{
		trap_SendServerCommand( ent-g_entities, va("print \"Client %s is not active\n\"", cmdTarget ) ); 
		return; 
	}

	if( !G_AdminControl( ent, &g_entities[clientid] ) )
	{
		trap_SendServerCommand(ent-g_entities, va("print \"^1You can't use this command on them. They are a higher admin level than you.\n\""));
		return;
	}

	if( !g_entities[clientid].client->sess.isSilenced )
	{
		g_entities[clientid].client->sess.isSilenced = qtrue;
	}

	trap_SendServerCommand(ent-g_entities, va("print \"^2Player silenced.\n\""));
	trap_SendServerCommand(clientid, va("cp \"^2You were silenced by an admin.\n\""));
	G_LogPrintf("Silence admin command executed by %s on %s.\n", ent->client->pers.netname, g_entities[clientid].client->pers.netname);
	return;
}

/*
============
amunsilence Function
============
*/
void Cmd_amUnSilence_F(gentity_t *ent)
{
	char cmdTarget[MAX_STRING_CHARS];
	int clientid = -1;

	if(!G_CheckAdmin(ent, ADMIN_SILENCE))
	{  	
		trap_SendServerCommand(ent-g_entities, va("print \"^1You are not allowed to use this command.\n\""));	  	
		return;  	
	}

	if(trap_Argc() < 2)
	{
		trap_SendServerCommand(ent-g_entities, va("print \"^2Command Usage: /amunsilence <name/clientid>\n\""));
		return;
	}

	trap_Argv(1, cmdTarget, MAX_STRING_CHARS);

	clientid = M_G_ClientNumberFromName( cmdTarget );
	if (clientid == -1) 
	{ 
		trap_SendServerCommand( ent-g_entities, va("print \"Can't find client ID for %s\n\"", cmdTarget ) ); 
		return; 
	} 
	if (clientid == -2) 
	{ 
		trap_SendServerCommand( ent-g_entities, va("print \"Ambiguous client ID for %s\n\"", cmdTarget ) ); 
		return; 
	}
	if (clientid >= MAX_CLIENTS || clientid < 0) 
	{ 
		trap_SendServerCommand( ent-g_entities, va("print \"Bad client ID for %s\n", cmdTarget ) );
		return;
	}
	if (!g_entities[clientid].inuse) 
	{
		trap_SendServerCommand( ent-g_entities, va("print \"Client %s is not active\n\"", cmdTarget ) ); 
		return; 
	}

	if( !g_entities[clientid].client->sess.isSilenced )
	{
		trap_SendServerCommand(ent-g_entities, va("print \"^2This player is not silenced.\n\""));
		return;
	}

	if( !G_AdminControl( ent, &g_entities[clientid] ) )
	{
		trap_SendServerCommand(ent-g_entities, va("print \"^1You can't use this command on them. They are a higher admin level than you.\n\""));
		return;
	}

	g_entities[clientid].client->sess.isSilenced = qfalse;
	
	trap_SendServerCommand(ent-g_entities, va("print \"^2Player unsilenced.\n\""));
	trap_SendServerCommand(clientid, va("cp \"^2You were unsilenced by an admin.\n\""));
	G_LogPrintf("Unsilence admin command executed by %s on %s.\n", ent->client->pers.netname, g_entities[clientid].client->pers.netname);
	return;
}

/*
============
amsleep Function
============
*/
void Cmd_amSleep_F(gentity_t *ent)
{
	char cmdTarget[MAX_STRING_CHARS];
	int clientid = -1;

	if(!G_CheckAdmin(ent, ADMIN_SLEEP))
	{  	
		trap_SendServerCommand(ent-g_entities, va("print \"^1You are not allowed to use this command.\n\""));	  	
		return;  	
	}

	if(trap_Argc() < 2)
	{
		trap_SendServerCommand(ent-g_entities, va("print \"^2Command Usage: /amsleep <name/clientid>\n\""));
		return;
	}

	trap_Argv(1, cmdTarget, MAX_STRING_CHARS);

	clientid = M_G_ClientNumberFromName( cmdTarget );
	if (clientid == -1) 
	{ 
		trap_SendServerCommand( ent-g_entities, va("print \"Can't find client ID for %s\n\"", cmdTarget ) ); 
		return; 
	} 
	if (clientid == -2) 
	{ 
		trap_SendServerCommand( ent-g_entities, va("print \"Ambiguous client ID for %s\n\"", cmdTarget ) ); 
		return; 
	}
	if (clientid >= MAX_CLIENTS || clientid < 0) 
	{ 
		trap_SendServerCommand( ent-g_entities, va("print \"Bad client ID for %s\n", cmdTarget ) );
		return;
	}
	if (!g_entities[clientid].inuse) 
	{
		trap_SendServerCommand( ent-g_entities, va("print \"Client %s is not active\n\"", cmdTarget ) ); 
		return; 
	}

	if( !G_AdminControl( ent, &g_entities[clientid] ) )
	{
		trap_SendServerCommand(ent-g_entities, va("print \"^1You can't use this command on them. They are a higher admin level than you.\n\""));
		return;
	}

	//TODO
	/*
	// MJN - are they in an emote?  Then unemote them :P
	if (InEmote(g_entities[clientid].client->emote_num ) || InSpecialEmote(g_entities[clientid].client->emote_num ))
	{
		G_SetTauntAnim(&g_entities[clientid], g_entities[clientid].client->emote_num);
	}
	*/

	if( !g_entities[clientid].client->sess.isSleeping )
	{
		g_entities[clientid].client->sess.isSleeping = qtrue;
	}
	else
	{
		trap_SendServerCommand( ent-g_entities, va( "print \"^1Player %s ^1is already sleeping. You can unsleep them with /amunsleep %s\n\"", g_entities[clientid].client->pers.netname, g_entities[clientid].client->pers.netname ) );
		return;
	}
	//TODO
	/*
	M_HolsterThoseSabers(&g_entities[clientid]);
	*/

	g_entities[clientid].client->ps.forceHandExtend = HANDEXTEND_KNOCKDOWN;
	g_entities[clientid].client->ps.forceDodgeAnim = 0;
	g_entities[clientid].client->ps.forceHandExtendTime = level.time + Q3_INFINITE;
	g_entities[clientid].client->ps.quickerGetup = qfalse;

	G_SetAnim( &g_entities[clientid], NULL, SETANIM_BOTH, BOTH_STUMBLEDEATH1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 0);
	
	trap_SendServerCommand( ent-g_entities, va( "print \"^2%s is now sleeping.\n\"", g_entities[clientid].client->pers.netname ) );
	trap_SendServerCommand( clientid, "cp \"^2You are now sleeping.\n\"" );

	G_LogPrintf("Sleep admin command executed by %s on %s.\n", ent->client->pers.netname, g_entities[clientid].client->pers.netname);
	return;
}

/*
============
amunsleep Function
============
*/
void Cmd_amUnsleep_F(gentity_t *ent)
{
	char cmdTarget[MAX_STRING_CHARS];
	int clientid = -1;

	if(!G_CheckAdmin(ent, ADMIN_SLEEP))
	{  	
		trap_SendServerCommand(ent-g_entities, va("print \"^1You are not allowed to use this command.\n\""));	  	
		return;  	
	}

	if(trap_Argc() < 2)
	{
		trap_SendServerCommand(ent-g_entities, va("print \"^2Command Usage: /amunsleep <name/clientid>\n\""));
		return;
	}

	trap_Argv(1, cmdTarget, MAX_STRING_CHARS);

	clientid = M_G_ClientNumberFromName( cmdTarget );
	if (clientid == -1) 
	{ 
		trap_SendServerCommand( ent-g_entities, va("print \"Can't find client ID for %s\n\"", cmdTarget ) ); 
		return; 
	} 
	if (clientid == -2) 
	{ 
		trap_SendServerCommand( ent-g_entities, va("print \"Ambiguous client ID for %s\n\"", cmdTarget ) ); 
		return; 
	}
	if (clientid >= MAX_CLIENTS || clientid < 0) 
	{ 
		trap_SendServerCommand( ent-g_entities, va("print \"Bad client ID for %s\n", cmdTarget ) );
		return;
	}
	if (!g_entities[clientid].inuse) 
	{
		trap_SendServerCommand( ent-g_entities, va("print \"Client %s is not active\n\"", cmdTarget ) ); 
		return; 
	}

	if( !g_entities[clientid].client->sess.isSleeping )
	{
		trap_SendServerCommand(ent-g_entities, va("print \"^2This player is not sleeping.\n\""));
		return;
	}

	if(g_entities[clientid].client->ps.duelInProgress)
	{
		trap_SendServerCommand(ent-g_entities, va("print \"^2You cannot use this command on someone who is duelling.\n\""));
		return;
	}

	if( !G_AdminControl( ent, &g_entities[clientid] ) )
	{
		trap_SendServerCommand(ent-g_entities, va("print \"^1You can't use this command on them. They are a higher admin level than you.\n\""));
		return;
	}

	g_entities[clientid].client->sess.isSleeping = qfalse;

	g_entities[clientid].client->ps.forceDodgeAnim = 0;
	g_entities[clientid].client->ps.forceHandExtendTime = 0;
	g_entities[clientid].client->ps.quickerGetup = qfalse;

	trap_SendServerCommand( ent-g_entities, va( "print \"^2%s has been unslept.\n\"", g_entities[clientid].client->pers.netname ) );

	trap_SendServerCommand( clientid, va("print \"^2You are no longer sleeping. You can get up by using a movement key.\n\"" ) );
	trap_SendServerCommand( clientid, va("cp \"^2You are no longer sleeping. You can get up by using a movement key.\n\"" ) );

	G_LogPrintf( "Unsleep admin command executed by %s on %s.\n", ent->client->pers.netname, g_entities[clientid].client->pers.netname );
	return;
}

/*
============
amaddeffect Function
============
*/
void Cmd_amEffect_F(gentity_t *ent)
{
	char	effect[MAX_STRING_CHARS]; // 16k file size
	gentity_t *fx_runner = G_Spawn();
	int i = 0;

	if(!G_CheckAdmin(ent, ADMIN_ADDEFFECT))
	{  	
		trap_SendServerCommand(ent-g_entities, va("print \"^1You are not allowed to use this command.\n\""));	  	
		return;  	
	}
	
	if ( trap_Argc() < 2 )
	{
		trap_SendServerCommand( ent-g_entities, va( "print \"^2Command Usage: /amaddeffect <effect> Example: /amaddeffect env/small_fire\n\"" ) );
		return;
	}

	trap_Argv( 1,  effect, MAX_STRING_CHARS );

	AddSpawnField("fxFile", effect);
#ifdef __LINUX__
	fx_runner->s.origin[2] = (int) ent->client->ps.origin[2];
#endif
#ifdef QAGAME
	fx_runner->s.origin[2] = (int) ent->client->ps.origin[2] - 15;
#endif
	fx_runner->s.origin[1] = (int) ent->client->ps.origin[1];
	fx_runner->s.origin[0] = (int) ent->client->ps.origin[0];
	SP_fx_runner(fx_runner);

	trap_SendServerCommand( ent-g_entities, va( "print \"^2Effect placed with ID: ^7%i.\n\"", fx_runner->s.number ) );

	for ( i = 0; i < 127; i++ )
	{
		if ( ent->client->sess.entListIDs[i] )
			continue;

		ent->client->sess.entListIDs[i] = fx_runner->s.number;
		Q_strncpyz( (char*)ent->client->sess.entListNames[i], effect, sizeof( ent->client->sess.entListNames[i] ) );
		break;
	}

	G_LogPrintf("Effect command executed by %s.\n", ent->client->pers.netname);
	return;
}

/*
============
amforceteam Function
============
*/
void Cmd_amForceTeam_F(gentity_t *ent)
{
	char teamname[MAX_STRING_CHARS], cmdTarget[MAX_STRING_CHARS];
	int clientid = -1;

	if(!G_CheckAdmin(ent, ADMIN_FORCETEAM))
	{  	
		trap_SendServerCommand(ent-g_entities, va("print \"^1You are not allowed to use this command.\n\""));	  	
		return;  	
	}
	
	if(trap_Argc() != 3) //If the user doesn't specify both args.
	{
		trap_SendServerCommand( ent-g_entities, va( "print \"^2Command Usage: /amforceteam <name/clientid> <newteam>\n\"" ) );
		return;
	}

	trap_Argv( 1, cmdTarget, MAX_STRING_CHARS ); //The first command argument is the target's name.

	trap_Argv( 2, teamname, MAX_STRING_CHARS ); //The second command argument is the team's name.

	clientid = M_G_ClientNumberFromName( cmdTarget );
	if (clientid == -1) 
	{ 
		trap_SendServerCommand( ent-g_entities, va("print \"Can't find client ID for %s\n\"", cmdTarget ) ); 
		return; 
	} 
	if (clientid == -2) 
	{ 
		trap_SendServerCommand( ent-g_entities, va("print \"Ambiguous client ID for %s\n\"", cmdTarget ) ); 
		return; 
	}
	if (clientid >= MAX_CLIENTS || clientid < 0) 
	{ 
		trap_SendServerCommand( ent-g_entities, va("print \"Bad client ID for %s\n", cmdTarget ) );
		return;
	}
	if (!g_entities[clientid].inuse) 
	{
		trap_SendServerCommand( ent-g_entities, va("print \"Client %s is not active\n\"", cmdTarget ) ); 
		return; 
	}

	if( !G_AdminControl( ent, &g_entities[clientid] ) )
	{
		trap_SendServerCommand(ent-g_entities, va("print \"^1You can't use this command on them. They are a higher admin level than you.\n\""));
		return;
	}
		
	/*
	if ( !Q_stricmp( teamname, "red" ) || !Q_stricmp( teamname, "r" ) ) {
		SetTeam( &g_entities[clientid], "red" );
		G_LogPrintf("ForceTeam [RED] admin command executed by %s on %s.\n", ent->client->pers.netname, g_entities[clientid].client->pers.netname);
	}
	else if ( !Q_stricmp( teamname, "blue" ) || !Q_stricmp( teamname, "b" ) ) {
		SetTeam( &g_entities[clientid], "blue" );
		G_LogPrintf("ForceTeam [BLUE] admin command executed by %s on %s.\n", ent->client->pers.netname, g_entities[clientid].client->pers.netname);
	}
	*/
	if ( !Q_stricmp( teamname, "spectate" ) || !Q_stricmp( teamname, "spectator" )  || !Q_stricmp( teamname, "spec" ) || !Q_stricmp( teamname, "s" ) ) {
		SetTeam( &g_entities[clientid], "spectator" );
		G_LogPrintf("ForceTeam [SPECTATOR] admin command executed by %s on %s.\n", ent->client->pers.netname, g_entities[clientid].client->pers.netname);
	}
	else if ( !Q_stricmp( teamname, "enter" ) || !Q_stricmp( teamname, "free" ) || !Q_stricmp( teamname, "join" ) || !Q_stricmp( teamname, "j" )
		 || !Q_stricmp( teamname, "f" ) ) {
		SetTeam( &g_entities[clientid], "free" );
		G_LogPrintf( "ForceTeam [FREE] admin command executed by %s on %s.\n", ent->client->pers.netname, g_entities[clientid].client->pers.netname );
	}
	trap_SendServerCommand( ent-g_entities, va( "print \"^2Player %s ^2was forceteamed.\n\"", g_entities[clientid].client->pers.netname ) );
	return;
}

/*
============
ammap Function
============
*/
void Cmd_amMap_F(gentity_t *ent)
{
	char map[MAX_STRING_CHARS];

	if(!G_CheckAdmin(ent, ADMIN_MAP))
	{  	
		trap_SendServerCommand(ent-g_entities, va("print \"^1You are not allowed to use this command.\n\""));	  	
		return;  	
	}

	if ( trap_Argc() < 2 )
	{
		trap_SendServerCommand( ent-g_entities, "print \"^2Command Usage: /amMap <mapName>\n\"" );
		return;
	}

	trap_Argv( 1, map, MAX_STRING_CHARS );

	trap_SendConsoleCommand( EXEC_APPEND, va("map %s\n", map));
	G_LogPrintf("Map changed to %s by %s.\n", map, ent->client->pers.netname);
	return;
}
/*
============
amweather Function
============
*/
void G_RemoveWeather( void ) //ensiform's whacky weather clearer code
{ 
	int i; 
	char s[MAX_STRING_CHARS]; 

	for (i=1 ; i<MAX_FX ; i++)
	{
		trap_GetConfigstring( CS_EFFECTS + i, s, sizeof( s ) );

		if (!*s || !s[0]) 
		{ 
			return;
		}

		if (s[0] == '*')
		{ 
			trap_SetConfigstring( CS_EFFECTS + i, ""); 
		}
	}
}

void Cmd_amWeather_F(gentity_t *ent)
{
	char weather[MAX_STRING_CHARS];
	int num;

	if(!G_CheckAdmin(ent, ADMIN_WEATHER))
	{  	
		trap_SendServerCommand(ent-g_entities, va("print \"^1You are not allowed to use this command.\n\""));	  	
		return;  	
	}

	if ( trap_Argc() < 2 )
	{
		trap_SendServerCommand( ent-g_entities, "print \"^2Command Usage: /amWeather <weather>\nTypes of weather: snow, rain, sandstorm, blizzard, fog, spacedust, acidrain\n\"" );
		return;
	}

	trap_Argv( 1,  weather, MAX_STRING_CHARS );

	trap_SendServerCommand( ent-g_entities, va( "print \"^2Changing the weather...\n\"" ) );
						
	if (!Q_stricmp(weather, "snow"))
	{
		G_RemoveWeather();
		num = G_EffectIndex("*clear");
		trap_SetConfigstring( CS_EFFECTS + num, "");
		G_EffectIndex("*snow");
	}
	else if (!Q_stricmp(weather, "rain"))
	{
		G_RemoveWeather();
		num = G_EffectIndex("*clear");
		trap_SetConfigstring( CS_EFFECTS + num, "");
		G_EffectIndex("*rain 500");
	}
	else if (!Q_stricmp(weather, "sandstorm"))
	{
		G_RemoveWeather();
		num = G_EffectIndex("*clear");
		trap_SetConfigstring( CS_EFFECTS + num, "");
		G_EffectIndex("*wind");
		G_EffectIndex("*sand");
	}
	else if (!Q_stricmp(weather, "blizzard"))
	{
		G_RemoveWeather();
		num = G_EffectIndex("*clear");
		trap_SetConfigstring( CS_EFFECTS + num, "");
		G_EffectIndex("*constantwind (100 100 -100)");
		G_EffectIndex("*fog");
		G_EffectIndex("*snow");
	}
	else if (!Q_stricmp(weather, "fog"))
	{
		G_RemoveWeather();
		num = G_EffectIndex("*clear");
		trap_SetConfigstring( CS_EFFECTS + num, "");
		G_EffectIndex("*heavyrainfog");
	}
	else if (!Q_stricmp(weather, "spacedust"))
	{
		G_RemoveWeather();
		num = G_EffectIndex("*clear");
		trap_SetConfigstring( CS_EFFECTS + num, "");
		G_EffectIndex("*spacedust 4000");
	}
	else if (!Q_stricmp(weather, "acidrain"))
	{
		G_RemoveWeather();
		num = G_EffectIndex("*clear");
		trap_SetConfigstring( CS_EFFECTS + num, "");
		G_EffectIndex("*acidrain 500");
	}

	else if (!Q_stricmp(weather, "clear"))
	{
		G_RemoveWeather();
		num = G_EffectIndex("*clear");
		trap_SetConfigstring( CS_EFFECTS + num, "");
	}

	else
	{
		trap_SendServerCommand( ent-g_entities, "print \"^1Invalid type of weather.\nTypes of weather: snow, rain, sandstorm, blizzard, fog, spacedust, acidrain\n\"" );
		return;
	}

	if (!Q_stricmp(weather, "clear"))
	{
		trap_SendServerCommand( ent-g_entities, va( "print \"^2Weather cleared.\n\"" ) );
		G_LogPrintf("Weather cleared by %s.\n", ent->client->pers.netname);
		return;
	}
	 
	else
	{
		trap_SendServerCommand( ent-g_entities, va( "print \"^2Weather changed to %s. To change it back, use /amweather clear\n\"", weather ) );
		G_LogPrintf("Weather command executed by %s. The weather is now %s.\n", ent->client->pers.netname, weather);
		return;
	}
}

//-----------
//WeatherPlus
//-----------
void Cmd_amWeatherPlus_F(gentity_t *ent)
{
	char weather[MAX_STRING_CHARS];
	int num;

	if(!G_CheckAdmin(ent, ADMIN_WEATHER))
	{  	
		trap_SendServerCommand(ent-g_entities, va("print \"^1You are not allowed to use this command.\n\""));	  	
		return;  	
	}

	if ( trap_Argc() < 2 )
	{
		trap_SendServerCommand( ent-g_entities, "print \"^2Command Usage: /amWeatherPlus <weather>\nTypes of weather: snow, rain, sandstorm, blizzard, fog, spacedust, acidrain\n\"" );
		return;
	}

	trap_Argv( 1,  weather, MAX_STRING_CHARS );
					
	if (!Q_stricmp(weather, "snow"))
	{
		trap_SetConfigstring( CS_EFFECTS, "");
		G_EffectIndex("*snow");
		trap_SendServerCommand( ent-g_entities,  "print \"^2Adding weather: \nSnow\n\"" ) ;
	}
	else if (!Q_stricmp(weather, "rain"))
	{
		trap_SetConfigstring( CS_EFFECTS, "");
		G_EffectIndex("*rain 500");
		trap_SendServerCommand( ent-g_entities,  "print \"^2Adding weather: \nSnow\n\"" ) ;
	}
	else if (!Q_stricmp(weather, "sandstorm"))
	{
		trap_SetConfigstring( CS_EFFECTS, "");
		G_EffectIndex("*wind");
		G_EffectIndex("*sand");
		trap_SendServerCommand( ent-g_entities,  "print \"^2Adding weather:\nSand\nWind\n\"" ) ;
	}
	else if (!Q_stricmp(weather, "blizzard"))
	{
		trap_SetConfigstring( CS_EFFECTS, "");
		G_EffectIndex("*constantwind (100 100 -100)");
		G_EffectIndex("*fog");
		G_EffectIndex("*snow");
		trap_SendServerCommand( ent-g_entities,  "print \"^2Adding weather:\nFog\nSnow\n\"" ) ;
	}
	else if (!Q_stricmp(weather, "heavyfog"))
	{
		trap_SetConfigstring( CS_EFFECTS, "");
		G_EffectIndex("*heavyrainfog");
		trap_SendServerCommand( ent-g_entities,  "print \"^2Adding weather:\nHeavy Fog\n\"" ) ;
	}
	else if (!Q_stricmp(weather, "spacedust"))
	{
		trap_SendServerCommand( ent-g_entities,  "print \"^2Adding weather:\nSpace Dust\n\"" ) ;
		trap_SetConfigstring( CS_EFFECTS, "");
		G_EffectIndex("*spacedust 4000");
	}
	else if (!Q_stricmp(weather, "acidrain"))
	{
		trap_SendServerCommand( ent-g_entities,  "print \"^2Adding weather:\nAcid Rain\n\"" ) ;
		trap_SetConfigstring( CS_EFFECTS, "");
		G_EffectIndex("*acidrain 500");
	}
	
	else if (!Q_stricmp(weather, "fog"))
	{
		trap_SetConfigstring( CS_EFFECTS, "");
		G_EffectIndex("*fog");
		trap_SendServerCommand( ent-g_entities,  "print \"^2Adding weather:\nFog\n\"" ) ;
	}
	else if (!Q_stricmp(weather, "sand"))
	{
		trap_SetConfigstring( CS_EFFECTS, "");
		G_EffectIndex("*sand");
		trap_SendServerCommand( ent-g_entities,  "print \"^2Adding weather:\nSand\n\"" ) ;
	}	
	else if (!Q_stricmp(weather, "clear"))
	{
		G_RemoveWeather();
		num = G_EffectIndex("*clear");
		trap_SetConfigstring( CS_EFFECTS + num, "");
	}
	else
	{
		trap_SendServerCommand( ent-g_entities, "print \"^1Invalid type of weather.\nTypes of weather: snow, rain, sandstorm, blizzard, fog, spacedust, acidrain\n\"" );
		return;
	}

	if (!Q_stricmp(weather, "clear"))
	{
		trap_SendServerCommand( ent-g_entities, va( "print \"^2Weather cleared.\n\"" ) );
		G_LogPrintf("Weather cleared by %s.\n", ent->client->pers.netname);
		return;
	}

	else
	{
		trap_SendServerCommand( ent-g_entities, va( "print \"^2Weather %s added. To clear the weather, use /amweatherplus clear\n\"", weather ) );
		G_LogPrintf("Weatherplus command executed by %s. The weather added was %s.\n", ent->client->pers.netname, weather);
		return;
	}
}
/*
============
amstatus Function
============
*/
void Cmd_amStatus_F(gentity_t *ent)
{
	char hasClientText[64];
	int i;

	if(!G_CheckAdmin(ent, ADMIN_STATUS))
	{  	
		trap_SendServerCommand(ent-g_entities, va("print \"^1You are not allowed to use this command.\n\""));	  	
		return;  	
	}

	trap_SendServerCommand( ent-g_entities, va( "print \"^2Status:\n\"" ) );
	for(i = 0; i < level.maxclients; i++)
	{ 
		if( g_entities[i].inuse && g_entities[i].client && g_entities[i].client->pers.connected == CON_CONNECTED )
		{
			if ( g_entities[i].client->sess.ojpClientPlugIn )
			{
				Q_strncpyz( hasClientText, "Yes", sizeof( hasClientText ) );
				trap_SendServerCommand( ent-g_entities, va( "print \"^2ID: ^7%i ^2Name: %s ^2IP: ^7%s ^2OpenRP Client: ^7%s\n\"", 
					i, g_entities[i].client->pers.netname, g_entities[i].client->sess.IP, hasClientText ) );
				/*trap_SendServerCommand( ent-g_entities, va( "print \"^2ID: ^7%i ^2Name: %s ^2IP: ^7%s ^2OpenRP Client: ^7%s ^2Version: ^7%s\n\"", 
					i, g_entities[i].client->pers.netname, g_entities[i].client->sess.IP, hasClientText, g_entities[i].client->sess.ojpClientVersion ) );*/
			}
			else
			{
				Q_strncpyz( hasClientText, "No", sizeof( hasClientText ) );
				trap_SendServerCommand( ent-g_entities, va( "print \"^2ID: ^7%i ^2Name: %s ^2IP: ^7%s ^2OpenRP Client: ^7%s\n\"", 
					i, g_entities[i].client->pers.netname, g_entities[i].client->sess.IP, hasClientText ) );
			}
			
		}
	}
	return;
}
/*
============
amrename Function
============
*/
void uwRename(gentity_t *player, const char *newname) 
{ 
   char userinfo[MAX_INFO_STRING]; 
   int clientNum = player-g_entities;
   trap_GetUserinfo(clientNum, userinfo, sizeof(userinfo)); 
   Info_SetValueForKey(userinfo, "name", newname);
   trap_SetUserinfo(clientNum, userinfo); 
   ClientUserinfoChanged(clientNum); 
   player->client->pers.netnameTime = level.time + 5000;
}

void uw2Rename(gentity_t *player, const char *newname) 
{ 
   char userinfo[MAX_INFO_STRING]; 
   int clientNum = player-g_entities;
   trap_GetUserinfo(clientNum, userinfo, sizeof(userinfo)); 
   Info_SetValueForKey(userinfo, "name", newname); 
   trap_SetUserinfo(clientNum, userinfo); 
   ClientUserinfoChanged(clientNum); 
   player->client->pers.netnameTime = level.time + Q3_INFINITE;
}

void Cmd_amRename_F(gentity_t *ent)
{
   char currentname[MAX_STRING_CHARS], newname[MAX_STRING_CHARS];
   int clientid = -1;

   if(!G_CheckAdmin(ent, ADMIN_RENAME))
	{  	
		trap_SendServerCommand(ent-g_entities, va("print \"^1You are not allowed to use this command.\n\""));	  	
		return;  	
	}

   if ( trap_Argc() != 3) 
   { 
      trap_SendServerCommand( ent-g_entities, va( "print \"^2Command Usage: /amrename <currentname> <newname>\n\"" ) ); 
      return;
   }

	trap_Argv( 1, currentname, MAX_STRING_CHARS );
	clientid = M_G_ClientNumberFromName( currentname );
	if (clientid == -1) 
	{ 
		trap_SendServerCommand( ent-g_entities, va("print \"Can't find client ID for %s\n\"", currentname ) ); 
		return; 
	} 
	if (clientid == -2) 
	{ 
		trap_SendServerCommand( ent-g_entities, va("print \"Ambiguous client ID for %s\n\"", currentname ) ); 
		return; 
	}
	if (clientid >= MAX_CLIENTS || clientid < 0) 
	{ 
		trap_SendServerCommand( ent-g_entities, va("print \"Bad client ID for %s\n", currentname ) );
		return;
	}
	if (!g_entities[clientid].inuse) 
	{
		trap_SendServerCommand( ent-g_entities, va("print \"Client %s is not active\n\"", currentname ) ); 
		return; 
	}

	if( !G_AdminControl( ent, &g_entities[clientid] ) )
	{
		trap_SendServerCommand(ent-g_entities, va("print \"^1You can't use this command on them. They are a higher admin level than you.\n\""));
		return;
	}

	trap_Argv( 2, newname, MAX_STRING_CHARS );
	G_LogPrintf("Rename admin command executed by %s on %s\n", ent->client->pers.netname, g_entities[clientid].client->pers.netname);
	trap_SendServerCommand(clientid, va("cvar name %s", newname));
	uwRename(&g_entities[clientid], newname);

	G_LogPrintf("Rename admin command executed by %s on %s.\n", ent->client->pers.netname, g_entities[clientid].client->pers.netname);
	return;
}
/*
=================

Grant Admin

=====
*/
void Cmd_GrantAdmin_F( gentity_t * ent )
{
	Database db(DATABASE_PATH);
	Query q(db);
	char username[MAX_TOKEN_CHARS], temp[MAX_STRING_CHARS], DBname[MAX_STRING_CHARS];
	int adminLevel, accountID;

	if(!G_CheckAdmin(ent, ADMIN_GRANTREMOVEADMIN))
	{  	
		trap_SendServerCommand(ent-g_entities, va("print \"^1You are not allowed to use this command.\n\""));	  	
		return;  	
	}

	if (!db.Connected())
	{
		G_Printf("Database not connected, %s\n",DATABASE_PATH);
		return;
	}

	if( trap_Argc() != 3 )
	{
		trap_SendServerCommand( ent-g_entities, "print \"^2Command Usage: /amGiveAdmin <username> <adminLevel>\n\"" );
		trap_SendServerCommand( ent-g_entities, "cp \"^2Command Usage: /amGiveAdmin <username> <adminLevel>\n\"" );
		return;
	}

	trap_Argv( 1, username, MAX_STRING_CHARS );
	//Check if this username exists
	Q_strlwr( username );

	Q_strncpyz( DBname,  q.get_string( va( "SELECT Username FROM Users WHERE Username='%s'", username ) ), sizeof( DBname ) );

	if( DBname[0] == '\0'  )
	{
		//The username does not exist, thus, the error does.
		trap_SendServerCommand ( ent-g_entities, va( "print \"^1Username %s does not exist.\n\"", username ) );
		trap_SendServerCommand ( ent-g_entities, va( "cp \"^1Username %s does not exist.\n\"", username ) );
		return;
	}

	trap_Argv( 2, temp, MAX_STRING_CHARS );

	adminLevel = atoi( temp );
	
	if ( adminLevel < 1 || adminLevel > 10 )
	{
		trap_SendServerCommand( ent-g_entities, "print \"The admin level must be a number from 1-10. 1 is the highest level, 10 is the lowest.\n\"" );
		return;
	}

	accountID = q.get_num( va( "SELECT AccountID FROM Users WHERE Username='%s'", username ) );

	if( !accountID )
	{
		trap_SendServerCommand( ent-g_entities, va( "print \"Username %s does not exist\n\"", username ) );
		return;
	}

	q.execute( va( "UPDATE Users set Admin='1' WHERE Username='%s'", username ) );

	q.execute( va( "UPDATE Users set AdminLevel='%i' WHERE Username='%s'", adminLevel, username ) );

	trap_SendServerCommand( ent-g_entities, va( "print \"^2Admin (level %i) given to %s.\n\"", adminLevel, username ) );
	return;
}

/*
=================

SV Grant Admin

=====
*/
void Cmd_SVGrantAdmin_F()
{
	Database db(DATABASE_PATH);
	Query q(db);
	char username[MAX_TOKEN_CHARS], temp[MAX_STRING_CHARS], DBname[MAX_STRING_CHARS];
	int adminLevel;

	if (!db.Connected())
	{
		G_Printf("Database not connected, %s\n",DATABASE_PATH);
		return;
	}

	if( trap_Argc() != 3 )
	{
		G_Printf( "Command Usage: giveAdmin <username> <adminLevel>\n" );
		return;
	}

	trap_Argv( 1, username, MAX_STRING_CHARS );
	//Check if this username exists
	Q_strlwr( username );

	Q_strncpyz( DBname, q.get_string( va( "SELECT Username FROM Users WHERE Username='%s'", username ) ), sizeof( DBname ) );

	if( DBname[0] == '\0' )
	{
		//The username does not exist, thus, the error does.
		G_Printf( "^1Username %s does not exist.\n", username );
		
	}

	trap_Argv( 2, temp, MAX_STRING_CHARS );

	adminLevel = atoi( temp );
	
	if ( adminLevel < 1 || adminLevel > 10 )
	{
		G_Printf( "Error: The admin level must be a number from 1-10.\n" );
		return;
	}

	q.execute( va( "UPDATE Users set Admin='1' WHERE Username='%s'", username ) );

	q.execute( va( "UPDATE Users set AdminLevel='%i' WHERE Username='%s'", adminLevel, username ) );

	G_Printf( "Admin (level %i) given to %s.\n", adminLevel, username );
	return;
}

/*
=================

Remove Admin

=====
*/
void Cmd_RemoveAdmin_F( gentity_t * ent )
{
	Database db(DATABASE_PATH);
	Query q(db);
	char username[MAX_TOKEN_CHARS];
	int valid;

	//OPENRPTODO - Add g_admincontrol?
	if(!G_CheckAdmin(ent, ADMIN_GRANTREMOVEADMIN))
	{  	
		trap_SendServerCommand(ent-g_entities, va("print \"^1You are not allowed to use this command.\n\""));	  	
		return;  	
	}

	if (!db.Connected())
	{
		G_Printf("Database not connected, %s\n",DATABASE_PATH);
		return;
	}

	if( trap_Argc() < 2 )
	{
		trap_SendServerCommand( ent-g_entities, "print \"^2Command Usage: /amRemoveAdmin <accountname>\n\"");
		return;
	}
	
	trap_Argv( 1, username, MAX_STRING_CHARS );

	Q_strlwr( username );
	
	valid = q.get_num( va( "SELECT AccountID FROM Users WHERE Username='%s'", username ) );

	if( !valid )
	{
		trap_SendServerCommand( ent-g_entities, va( "print \"Username %s does not exist\n\"", username ) );
		return;
	}

	q.execute( va( "UPDATE Users set Admin='0' WHERE Username='%s'", username ) );
	//Set their adminlevel to 11 just to be safe.
	q.execute( va( "UPDATE Users set adminlevel='11' WHERE Username='%s'", username ) );

	trap_SendServerCommand( ent-g_entities, va( "print \"Admin removed from account %s\n\"", username ) );
	return;
}

/*
=================

SV Remove Admin

=====
*/
void Cmd_SVRemoveAdmin_F()
{
	Database db(DATABASE_PATH);
	Query q(db);
	char username[MAX_TOKEN_CHARS];
	int valid;

	if (!db.Connected())
	{
		G_Printf( "Database not connected, %s\n", DATABASE_PATH );
		return;
	}

	if( trap_Argc() < 2 )
	{
		G_Printf( "Command Usage: removeAdmin <accountname>\n" );
		return;
	}

	trap_Argv( 1, username, MAX_STRING_CHARS );

	Q_strlwr( username );

	valid = q.get_num( va( "SELECT AccountID FROM Users WHERE Username='%s'", username ) );

	if( !valid )
	{
		G_Printf( "Username %s does not exist\n\"", username );
		return;
	}

	q.execute( va( "UPDATE Users set Admin='0' WHERE Username='%s'", username ) );
	//Set their adminlevel to 11 just to be safe.
	q.execute( va( "UPDATE Users set AdminLevel='11' WHERE Username='%s'", username ) );

	G_Printf( "Admin removed from account %s.\n", username );
	return;
}

/*
=================

Give Skill Points

=====
*/
void Cmd_GiveSkillPoints_F(gentity_t * ent)
{
	Database db(DATABASE_PATH);
	Query q(db);
	char charName[MAX_STRING_CHARS], temp[MAX_STRING_CHARS];
	int changedSkillPoints;
	int charID;
	int accountID;
	int clientID;
	int loggedIn;
	int currentLevel;
	int currentSkillPoints;
	int newSkillPointsTotal;
	int i;
	extern void LevelCheck( int charID );

	if(!G_CheckAdmin(ent, ADMIN_SKILLPOINTS))
	{  	
		trap_SendServerCommand(ent-g_entities, va("print \"^1You are not allowed to use this command.\n\""));	  	
		return;  	
	}

	if ( !db.Connected() )
	{
		G_Printf( "Database not connected, %s\n", DATABASE_PATH );
		return;
	}

	if( trap_Argc() < 2 )
	{
		trap_SendServerCommand( ent-g_entities, "print \"^2Command Usage: /amGiveSkillPoints <characterName> <skillPoints>\n\"" );
		return;
	}

	//Character name
	trap_Argv( 1, charName, MAX_STRING_CHARS );

	//XP Added or removed.
	trap_Argv( 2, temp, MAX_STRING_CHARS );
	changedSkillPoints = atoi(temp);

	//Check if the character exists
	for( i = 0; charName[i]; i++ )
	{
		charName[i] = tolower( charName[i] );
	}

	charID = q.get_num( va( "SELECT CharID FROM Characters WHERE Name='%s'", charName ) );

	if( !charID )
	{
		trap_SendServerCommand( ent-g_entities, va( "print \"^1Character %s does not exist.\n\"", charName ) );
		trap_SendServerCommand( ent-g_entities, va( "cp \"^1Character %s does not exist.\n\"", charName ) );
		return;
	}

	//Get their accountID
	accountID = q.get_num( va( "SELECT AccountID FROM Characters WHERE CharID='%i'", charID ) );
	//Get their clientID so we can send them messages
	clientID = q.get_num( va( "SELECT ClientID FROM Users WHERE AccountID='%i'", accountID ) );
	loggedIn = q.get_num( va( "SELECT LoggedIn FROM Users WHERE AccountID='%i'", accountID ) );

	currentLevel = q.get_num( va( "SELECT Level FROM Characters WHERE CharID='%i'", charID ) );

	currentSkillPoints = q.get_num( va( "SELECT SkillPoints FROM Characters WHERE CharID='%i'", charID ) );

	newSkillPointsTotal = currentSkillPoints + changedSkillPoints;

	if ( newSkillPointsTotal < 1 )
	{
		trap_SendServerCommand( ent-g_entities, va ( "print \"^1You can't give %i skill points to %s, or else they would have < 1 skill point.\n\"", changedSkillPoints ) );
		return;
	}

	q.execute( va( "UPDATE Characters set SkillPoints='%i' WHERE CharID='%i'", newSkillPointsTotal, charID ) );

	switch( currentLevel )
	{
		case 50:
			if ( loggedIn )
			{
				AddSkill( &g_entities[clientID], changedSkillPoints );
				trap_SendServerCommand( clientID, "localSound sound/OpenRP/xp.mp3" );
				trap_SendServerCommand( clientID, va( "print \"^2You received %i skill points! You are the highest level, so you won't level up anymore!\n\"", changedSkillPoints ) );
			}
			break;
		default:
			if ( loggedIn )
			{
				AddSkill( &g_entities[clientID], changedSkillPoints );
				trap_SendServerCommand( clientID, "localSound sound/OpenRP/xp.mp3" );
				trap_SendServerCommand( clientID, va( "print \"^2You received %i skill points!\n\"", changedSkillPoints ) );
			}
			LevelCheck(charID);
			break;
	}

	trap_SendServerCommand( ent-g_entities, va( "print \"^2%i skill points have been given to character %s.\n\"", changedSkillPoints, charName ) );
	trap_SendServerCommand( ent-g_entities, va( "cp \"^2%i skill points have been given to character %s.\n\"", changedSkillPoints, charName ) );
	return;
}

/*
=================

Generate Credits

=====
*/
void Cmd_GenerateCredits_F(gentity_t * ent)
{
	Database db(DATABASE_PATH);
	Query q(db);
	char charName[MAX_STRING_CHARS], temp[MAX_STRING_CHARS];
	int changedCredits;
	int charID;
	int accountID;
	int clientID;
	int loggedIn;
	int currentCredits;
	int newCreditsTotal;

	if(!G_CheckAdmin(ent, ADMIN_CREDITS))
	{  	
		trap_SendServerCommand(ent-g_entities, va("print \"^1You are not allowed to use this command.\n\""));	  	
		return;  	
	}

	if ( !db.Connected() )
	{
		G_Printf( "Database not connected, %s\n", DATABASE_PATH );
		return;
	}

	if( trap_Argc() < 2 )
	{
		trap_SendServerCommand( ent-g_entities, "print \"^2Command Usage: /amGenCredits <characterName> <amount>\n\"" );
		trap_SendServerCommand( ent-g_entities, "cp \"^2Command Usage: /amGenCredits <characterName> <amount>\n\"" );
		return;
	}

	//Character name
	trap_Argv( 1, charName, MAX_STRING_CHARS );

	//Credits Added or removed.
	trap_Argv( 2, temp, MAX_STRING_CHARS );
	changedCredits = atoi( temp );

	Q_strlwr( charName );

	charID = q.get_num( va( "SELECT CharID FROM Characters WHERE Name='%s'", charName ) );

	if( !charID )
	{
		trap_SendServerCommand( ent-g_entities, va( "print \"^1Character %s does not exist.\n\"", charName ) );
		trap_SendServerCommand( ent-g_entities, va( "cp \"^1Character %s does not exist.\n\"", charName) );
		return;
	}

	//Get their accountID
	accountID = q.get_num( va( "SELECT AccountID FROM Characters WHERE CharID='%i'", charID ) );
	//Get their clientID so we can send them messages
	clientID = q.get_num( va( "SELECT ClientID FROM Users WHERE AccountID='%i'", accountID ) );
	loggedIn = q.get_num( va( "SELECT LoggedIn FROM Users WHERE AccountID='%i'", accountID ) );

	currentCredits = q.get_num( va( "SELECT Credits FROM Characters WHERE CharID='%i'", charID ) );

	newCreditsTotal = currentCredits + changedCredits;

	q.execute( va( "UPDATE Characters set Credits='%i' WHERE CharID='%i'", newCreditsTotal, charID ) );

	if( loggedIn )
	{
		trap_SendServerCommand( clientID, va( "print \"^2You received some credits from an admin!\n\"", charName ) );
		trap_SendServerCommand( clientID, va( "cp \"^2You received some credits from an admin!\n\"", charName ) );
	}

	trap_SendServerCommand( ent-g_entities, va( "print \"^2%i credits have been generated and given to character %s.\n\"", changedCredits, charName ) );
	trap_SendServerCommand( ent-g_entities, va( "cp \"^2%i credits have been generated and given to character %s.\n\"", changedCredits, charName ) );
	return;
}

/*
=================

Create Faction

=====
*/
void Cmd_CreateFaction_F(gentity_t * ent)
{
	Database db(DATABASE_PATH);
	Query q(db); 
	char factionName[MAX_STRING_CHARS], DBname[MAX_STRING_CHARS];
	int charCurrentFactionID, factionID;

	if(!G_CheckAdmin(ent, ADMIN_FACTION))
	{  	
		trap_SendServerCommand(ent-g_entities, va("print \"^1You are not allowed to use this command.\n\""));	  	
		return;  	
	}

	if( !ent->client->sess.characterChosen )
	{
		trap_SendServerCommand( ent-g_entities, "print \"^1You must be logged in and have a character selected in order to use this command.\n\"" );
		return;
	}

	if ( !db.Connected() )
	{
		G_Printf( "Database not connected, %s\n", DATABASE_PATH );
		return;
	}

	charCurrentFactionID = q.get_num( va( "SELECT FactionID FROM Characters WHERE CharID='%i'", ent->client->sess.characterID ) );

	if ( charCurrentFactionID > 0)
	{
		trap_SendServerCommand( ent-g_entities, va( "print \"^1You must leave your current faction first before creating a faction.\n\"" ) );
		return;
	}

	if( trap_Argc() < 2 )
	{
		trap_SendServerCommand( ent-g_entities, "print \"^2Command Usage: /amCreateFaction <factionName>\n\"" );
		return;
	}

	trap_Argv( 1, factionName, MAX_STRING_CHARS );

	Q_strlwr( factionName );

	Q_strncpyz( DBname, q.get_string( va( "SELECT Name FROM Factions WHERE Name='%s'", factionName) ), sizeof( DBname ) );
	if( DBname[0] != '\0' )
	{
		trap_SendServerCommand ( ent-g_entities, va( "print \"^1Faction %s already exists.\n\"", DBname ) );
		trap_SendServerCommand ( ent-g_entities, va( "cp \"^1Faction %s already exists.\n\"", DBname ) );
		return;
	}
	
	q.execute(va("INSERT INTO Factions(Name,Bank) VALUES('%s','0')", factionName ) );
	factionID = q.get_num( va( "SELECT FactionID FROM Factions WHERE Name='%s'", factionName ) );
	q.execute( va( "UPDATE Characters set FactionID='%i' WHERE CharID='%i'", factionID, ent->client->sess.characterID ) );
	q.execute( va( "UPDATE Characters set FactionRank='Leader' WHERE CharID='%i'", ent->client->sess.characterID ) );
	trap_SendServerCommand( ent-g_entities, va( "print \"^2The %s faction has been created. To add people to it, use /setFaction %i <characterName>\n\"", factionName, factionID ) );
	return;
}

/*
=================

Set Faction

=====
*/
void Cmd_SetFaction_F( gentity_t * ent )
{
	Database db(DATABASE_PATH);
	Query q(db);
	char charName[MAX_STRING_CHARS], factionIDTemp[MAX_STRING_CHARS], factionName[MAX_STRING_CHARS];
	int charID;
	int accountID;
	int clientID;
	int loggedIn;
	int factionID;

	if(!G_CheckAdmin(ent, ADMIN_FACTION))
	{  	
		trap_SendServerCommand(ent-g_entities, va("print \"^1You are not allowed to use this command.\n\""));	  	
		return;  	
	}

	if ( !db.Connected() )
	{
		G_Printf( "Database not connected, %s\n", DATABASE_PATH );
		return;
	}

	if ( trap_Argc() != 3 )
	{
		trap_SendServerCommand( ent-g_entities, "print \"^2Command Usage: /amSetFaction <characterName> <factionName>\n\"" );
		return;
	}

	trap_Argv( 1, charName, MAX_STRING_CHARS );

	trap_Argv( 2, factionIDTemp, MAX_STRING_CHARS );
	factionID = atoi( factionIDTemp );

	Q_strlwr( charName );

	charID = q.get_num( va( "SELECT CharID FROM Characters WHERE Name='%s'", charName ) );

	if( !charID )
	{
		trap_SendServerCommand( ent-g_entities, va( "print \"^1Character %s does not exist.\n\"", charName ) );
		return;
	}

	//Get their accountID
	accountID = q.get_num( va( "SELECT AccountID FROM Characters WHERE CharID='%i'", charID ) );
	//Get their clientID so we can send them messages
	clientID = q.get_num( va( "SELECT ClientID FROM Users WHERE AccountID='%i'", accountID ) );
	loggedIn = q.get_num( va( "SELECT LoggedIn FROM Users WHERE AccountID='%i'", accountID ) );

	if (!Q_stricmp(factionIDTemp, "none"))
	{
		q.execute( va( "UPDATE Characters set FactionID='0' WHERE CharID='%i'", charID ) );
		q.execute( va( "UPDATE Characters set FactionRank='none' WHERE CharID='%i'", charID ) );

		if ( loggedIn )
		{
			trap_SendServerCommand( clientID, "print \"^2You have been removed from your faction.\n\"" );
			trap_SendServerCommand( clientID, "cp \"^2You have been removed from your faction.\n\"" );
		}
		trap_SendServerCommand( ent-g_entities, va( "print \"^2Character %s has been removed from their faction.\n\"", charName ) );
	}
	else
	{
		Q_strncpyz( factionName, q.get_string( va( "SELECT Name FROM Factions WHERE ID='%i'", factionID ) ), sizeof( factionName ) );
		q.execute( va( "UPDATE Characters set FactionID='%i' WHERE CharID='%i'", factionID, charID ) );
		q.execute( va( "UPDATE Characters set FactionRank='Member' WHERE CharID='%i'", charID ) );

		if ( loggedIn )
		{
			trap_SendServerCommand( clientID, va( "print \"^2You have been put in the %s faction!\nYou can use /factionInfo to view info about it.\n\"", factionName ) );
			trap_SendServerCommand( clientID, va( "cp \"^2You have been put in the %s faction!\n^2You can use /factionInfo to view info about it.\n\"", factionName ) );
		}

		trap_SendServerCommand( ent-g_entities, va( "print \"^2Character %s has been put in the %s faction.\nUse /setFactionRank to change their rank. Is it currently set to: Member\n\"", charName, factionName ) );
	}
	return;
}

/*
=================

Faction Generate Credits

=====
*/
void Cmd_FactionGenerateCredits_F(gentity_t * ent)
{
	Database db(DATABASE_PATH);
	Query q(db);
	char temp[MAX_STRING_CHARS], temp2[MAX_STRING_CHARS], factionName[MAX_STRING_CHARS];
	int factionID, changedCredits;
	int currentCredits;
	int newCreditsTotal;

	if(!G_CheckAdmin(ent, ADMIN_FACTION) && !G_CheckAdmin(ent, ADMIN_CREDITS))
	{  	
		trap_SendServerCommand(ent-g_entities, va("print \"^1You are not allowed to use this command.\n\""));	  	
		return;  	
	}

	if(!G_CheckAdmin(ent, ADMIN_CREDITS) && !G_CheckAdmin(ent, ADMIN_FACTION))
	{
		trap_SendServerCommand(ent-g_entities, va("print \"^1You are not allowed to use this command.\n\""));
		return;
	}

	if ( !db.Connected() )
	{
		G_Printf( "Database not connected, %s\n", DATABASE_PATH );
		return;
	}

	if( trap_Argc() < 2 )
	{
		trap_SendServerCommand( ent-g_entities, "print \"^2Command Usage: /amFactionGenCredits <factionID> <amount>\n\"" );
		return;
	}

	//Faction ID
	trap_Argv( 1, temp, MAX_STRING_CHARS );
	factionID = atoi( temp );

	//Credits Added or removed.
	trap_Argv( 2, temp2, MAX_STRING_CHARS );
	changedCredits = atoi( temp2 );

	//Check if the faction exists
	if ( !factionID )
	{
		trap_SendServerCommand( ent-g_entities, va( "print \"^1Faction with FactionID %i does not exist.\n\"", factionID ) );
		trap_SendServerCommand( ent-g_entities, va( "cp \"^1Faction with FactionID %i does not exist.\n\"", factionID ) );
		return;
	}

	
	Q_strncpyz( factionName, q.get_string( va( "SELECT Name FROM Factions WHERE FactionID='%i'", factionID ) ), sizeof( factionName ) );

	currentCredits = q.get_num( va( "SELECT Bank FROM Factions WHERE FactionID='%i'", factionID ) );

	newCreditsTotal = currentCredits + changedCredits;

	q.execute( va( "UPDATE Factions set Bank='%i' WHERE FactionID='%i'", newCreditsTotal, factionID ) );

	trap_SendServerCommand( ent-g_entities, va( "print \"^2%i credits have been generated and given to faction %s.\n\"", changedCredits, factionName ) );
	trap_SendServerCommand( ent-g_entities, va( "cp \"^2%i credits have been generated and given to faction %s.\n\"", changedCredits, factionName ) );
	return;
}

void Cmd_ShakeScreen_F( gentity_t * ent )
{
	int i;
	char temp[MAX_STRING_CHARS], temp2[MAX_STRING_CHARS];
	int intensity, length;

	if(!G_CheckAdmin(ent, ADMIN_SHAKE))
	{  	
		trap_SendServerCommand(ent-g_entities, va("print \"^1You are not allowed to use this command.\n\""));	  	
		return;  	
	}

	if ( trap_Argc() != 3 )
	{
		trap_SendServerCommand( ent-g_entities, "print \"^2Command Usage: /amShakeScreen <intensity> <length>\nExample: /amShakeScreen 5 7\"" );
		return;
	}

	trap_Argv( 1, temp, MAX_STRING_CHARS );
	intensity = atoi( temp );
	trap_Argv( 2, temp2, MAX_STRING_CHARS );
	length = atoi( temp2 ) * 1000;


	for( i = 0; i < level.maxclients; i++ )
	{
		if( g_entities[i].inuse && g_entities[i].client && g_entities[i].client->pers.connected == CON_CONNECTED )
		{
			G_ScreenShake( g_entities[i].s.origin, &g_entities[i], intensity, length, qtrue );
			//Don't do a center print for the target - it would distract from the shaking screen.
			trap_SendServerCommand( i, "print \"^3An admin shook your screen.\n\"" );
		}
	}
	trap_SendServerCommand( ent-g_entities, "print \"^2You shook everybody's screen.\n\"" );
	return;
}


void Cmd_Audio_F( gentity_t * ent )
{
	char audioPath[MAX_STRING_CHARS];
	int i;

	if(!G_CheckAdmin(ent, ADMIN_AUDIO))
	{  	
		trap_SendServerCommand(ent-g_entities, va("print \"^1You are not allowed to use this command.\n\""));	  	
		return;  	
	}

	if(trap_Argc() < 2)
	{
		trap_SendServerCommand(ent-g_entities, va("print \"^2Command Usage: /amAudio <path>\nYou can also use /amAudio alert to play an alarm, or /amAudio none to turn off the map music.\n\""));
		return;
	}

	trap_Argv(1, audioPath, MAX_STRING_CHARS);

	if( !Q_stricmpn( audioPath, "music/", 6 ) )
	{
		trap_SendServerCommand( ent-g_entities, va( "print \"^2You started playing the music file: ^7%s\n\"", audioPath ) );
		trap_SetConfigstring( CS_MUSIC, audioPath );
	}

	else if( !Q_stricmpn(audioPath, "sound/", 6 ) )
	{
		trap_SendServerCommand( ent-g_entities, va( "print \"^2You started playing the sound file: ^7%s\n\"", audioPath ) );
		for( i = 0; i < level.maxclients; i++ )
		{
			if( g_entities[i].inuse && g_entities[i].client && g_entities[i].client->pers.connected == CON_CONNECTED )
			{
				G_Sound( &g_entities[i], CHAN_MUSIC, G_SoundIndex( va( "%s", audioPath ) ) );
			}
		}
	}

	else if( !Q_stricmpn(audioPath, "alert", 5 ) )
	{
		trap_SendServerCommand( ent-g_entities, "print \"^2You started playing the alert sound file.\n\"" );
		for( i = 0; i < level.maxclients; i++ )
		{
			if( g_entities[i].inuse && g_entities[i].client && g_entities[i].client->pers.connected == CON_CONNECTED )
			{
				G_Sound( &g_entities[i], CHAN_MUSIC, G_SoundIndex( "sound/OpenRP/alert.mp3" ) );
			}
		}
	}
	else if ( !Q_stricmpn( audioPath, "none", 4 ) )
	{
		trap_SetConfigstring( CS_MUSIC, "" );
		trap_SendServerCommand( ent-g_entities, "print \"^2You turned off the map music.\n\"" );
	}
	else
	{
		trap_SendServerCommand( ent-g_entities, "print \"^1Music or sound must be in the music or sound folders, or you're just using the command incorrectly.\n\"" );
	}
	return;
}

void Cmd_amTelemark_F( gentity_t * ent )
{
	if(!G_CheckAdmin(ent, ADMIN_TELEPORT))
	{  	
		trap_SendServerCommand(ent-g_entities, va("print \"^1You are not allowed to use this command.\n\""));	  	
		return;  	
	}

	// mark the teleportation point with a mark.
	ent->client->ps.viewangles[0] = 0.0f;
	ent->client->ps.viewangles[2] = 0.0f;
	ent->client->pers.amtelemark1 = (int) ent->client->ps.origin[0];
	ent->client->pers.amtelemark2 = (int) ent->client->ps.origin[1];
	ent->client->pers.amtelemark3 = (int) ent->client->ps.origin[2];
	ent->client->pers.amtelemarkyaw = ent->client->ps.viewangles[1];
	ent->client->pers.amtelemarkset = qtrue;
	trap_SendServerCommand( ent-g_entities, va("print \"^2Telemark placed at: X:^7%d, ^2Y:^7%d, ^2Z:^7%d, ^2YAW:^7%d\n\"", (int) ent->client->ps.origin[0], (int) ent->client->ps.origin[1], (int) ent->client->ps.origin[2], ent->client->pers.amtelemarkyaw));
	return;
}

void Cmd_amOrigin_F( gentity_t * ent )
{
	int	clientid = -1; 
	char	arg1[MAX_STRING_CHARS];

	if(!G_CheckAdmin(ent, ADMIN_TELEPORT))
	{  	
		trap_SendServerCommand(ent-g_entities, va("print \"^1You are not allowed to use this command.\n\""));	  	
		return;  	
	}

	trap_Argv( 1, arg1, sizeof( arg1 ) );
	clientid = M_G_ClientNumberFromName( arg1 );

	//cm - Dom
	//BugFix: If you gave an ambigious name (e.g. The letter 'a' appears both in XharocK and Alora)
	//to this command it would crash the server.
	if (clientid == -1)
	{
		trap_SendServerCommand( ent-g_entities, va("print \"Can't find client ID for %s\n\"", arg1 ) );
		return;
	}
	if (clientid == -2)
	{
		trap_SendServerCommand( ent-g_entities, va("print \"Ambiguous client ID for %s\n\"", arg1 ) );
		return;
	}

	if (clientid)
	{
		trap_SendServerCommand( ent-g_entities, va("print \"^2X:^7%d, ^2Y:^7%d, ^2Z:^7%d\n\"", (int) g_entities[clientid].client->ps.origin[0], (int) g_entities[clientid].client->ps.origin[1], (int) g_entities[clientid].client->ps.origin[2]));
		return;
	}
	else
	{
		trap_SendServerCommand( ent-g_entities, va("print \"^2X:^7%d, ^2Y:^7%d, ^2Z:^7%d\n\"", (int) ent->client->ps.origin[0], (int) ent->client->ps.origin[1], (int) ent->client->ps.origin[2]));
	}
	return;
}

void Cmd_AdminChat_F( gentity_t *ent )
{
	int pos = 0;
	char real_msg[MAX_STRING_CHARS];
	char *msg = ConcatArgs(1);
	int i;

	if ( !ent->client->sess.isAdmin )
	{
		trap_SendServerCommand(ent-g_entities, va("print \"^1You are not allowed to use this command.\n\""));
		return;
	}

	while(*msg)
	{
		if(msg[0] == '\\' && msg[1] == 'n')
		{ 
			msg++;
			real_msg[pos++] = '\n';
		}
		else
		{
			real_msg[pos++] = *msg;
		}
		msg++;
	}

	real_msg[pos] = 0;

	for ( i = 0; i < level.maxclients; i++ )
	{
		if ( g_entities[i].client->sess.isAdmin && g_entities[i].inuse && g_entities[i].client && g_entities[i].client->pers.connected == CON_CONNECTED )
		{
			trap_SendServerCommand( i, va ("chat \"^6<Admin Chat> ^7%s^6: ^6%s\"", ent->client->pers.netname, real_msg ) );
		}
	}
	return;
}

void Cmd_AllChat_F( gentity_t * ent )
{
	char parameter[MAX_STRING_CHARS];

	if(!G_CheckAdmin(ent, ADMIN_ALLCHAT))
	{  	
		trap_SendServerCommand(ent-g_entities, va("print \"^1You are not allowed to use this command.\n\""));	  	
		return;  	
	}

	if ( trap_Argc() < 2 )
	{
		if ( !ent->client->sess.allChat )
		{
			if ( ent->client->sess.allChatComplete )
			{
				ent->client->sess.allChatComplete = qfalse;
				trap_SendServerCommand( ent-g_entities, "print \"^2All chat COMPLETE turned OFF.\n\"" );
			}
			ent->client->sess.allChat =	qtrue;
			trap_SendServerCommand( ent-g_entities, "print \"^2All chat turned ON.\n\"" );
			return;
		}
		else
		{
			ent->client->sess.allChat =	qfalse;
			trap_SendServerCommand( ent-g_entities, "print \"^2All chat turned OFF.\n\"" );
			return;
		}
	}

	trap_Argv( 1, parameter, MAX_STRING_CHARS );

	if( !Q_stricmp( parameter, "complete" ) )
	{
		if ( ent->client->sess.allChat )
		{
			ent->client->sess.allChat = qfalse;
			trap_SendServerCommand( ent-g_entities, "print \"^2All chat turned OFF.\n\"" );
		}
		
		if ( !ent->client->sess.allChatComplete )
		{
			ent->client->sess.allChatComplete = qtrue;
			trap_SendServerCommand( ent-g_entities, "print \"^2All chat COMPLETE turned ON.\n\"" );
		}
		else
		{
			ent->client->sess.allChatComplete = qfalse;
			trap_SendServerCommand( ent-g_entities, "print \"^2All chat COMPLETE turned OFF.\n\"" );
		}
	}
	return;
}

void Cmd_amWarningList_F(gentity_t *ent)
{
	int i;

	if(!G_CheckAdmin(ent, ADMIN_WARN))
	{  	
		trap_SendServerCommand(ent-g_entities, va("print \"^1You are not allowed to use this command.\n\""));	  	
		return;  	
	}

	trap_SendServerCommand( ent-g_entities, "print \"^2Warning List:\n\n\"" );
	for ( i = 0; i < level.maxclients; i++ )
	{
		if( g_entities[i].inuse && g_entities[i].client && g_entities[i].client->pers.connected == CON_CONNECTED )
		{
			trap_SendServerCommand( ent-g_entities, va( "print \"^7%s ^2%i/%i\n\"", g_entities[i].client->pers.netname, g_entities[i].client->sess.warnings, openrp_maxWarnings.integer ) );
		}
	}
	return;
}

//Thanks to Raz0r for posting this command.
void Cmd_SpawnEnt_F( gentity_t *ent )
{
	gentity_t *obj;
	char buf[32]; // arg1
	trace_t tr; //For tracing
	vec3_t fPos; //We're going to adjust the tr.endpos, and put it in here
	int i; //For looping 
	char spawnflags[MAX_STRING_CHARS];

	if(!G_CheckAdmin(ent, ADMIN_BUILD))
	{  	
		trap_SendServerCommand(ent-g_entities, va("print \"^1You are not allowed to use this command.\n\""));	  	
		return;  	
	}

	if ( trap_Argc() < 2 )
	{
		trap_SendServerCommand( ent-g_entities, "print \"^2Command Usage: /amSpawnEnt <entName>\n\"" );
		return;
	}

	//Trace to where we're looking
	AngleVectors( ent->client->ps.viewangles, fPos, 0, 0 );
	for ( i = 0; i < 3 ; i++ )
	{
		fPos[i] = ent->client->ps.origin[i] + fPos[i]*Q3_INFINITE;
	}
	trap_Trace( &tr, ent->client->ps.origin, 0, 0, fPos, ent->s.number, ent->clipmask );

	//Move along the normal of the traced plane
	VectorMA( tr.endpos, 48.0f, tr.plane.normal, fPos );

	//Grab the first argument (so /spawn misc_ammo_floor_unit)
	trap_Argv( 1, buf, sizeof( buf ) );

	trap_Argv( 2, spawnflags, sizeof( spawnflags ) );

	if ( spawnflags[0] != '\0' )
	{
	
		strcpy(level.spawnVars[0][0], "spawnflags");
		strcpy(level.spawnVars[0][1], va("%s", spawnflags ));
		level.numSpawnVars++;

		obj = G_Spawn(); //This will give us the first free slot in g_entities!
		//Now we have to set the classname and the origin
		//Then we tell the game to find the appropriate spawn function for the classname/entity
		obj->classname = buf;
		VectorCopy( fPos, obj->s.origin );
		G_CallSpawn( obj ); // Will search through the list of known entities and react accordingly

		trap_SendServerCommand( ent-g_entities, va( "print \"^2Ent with ID: ^7%i ^2and spawnflags: ^7%s ^2spawned.\n\"", obj->s.number, spawnflags ) );
		for ( i = 0; i < 127; i++ )
		{
			if ( ent->client->sess.entListIDs[i] )
				continue;

			ent->client->sess.entListIDs[i] = obj->s.number;
			Q_strncpyz( (char*)ent->client->sess.entListNames[i], buf, sizeof( ent->client->sess.entListNames[i] ) );
			break;
		}
	}
	else
	{
		obj = G_Spawn(); //This will give us the first free slot in g_entities!
		//Now we have to set the classname and the origin
		//Then we tell the game to find the appropriate spawn function for the classname/entity
		obj->classname = buf;
		VectorCopy( fPos, obj->s.origin );
		G_CallSpawn( obj ); // Will search through the list of known entities and react accordingly

		trap_SendServerCommand( ent-g_entities, va( "print \"^2Ent with ID: ^7%i ^2spawned.\n\"", obj->s.number ) );

		for ( i = 0; i < 127; i++ )
		{
			if ( ent->client->sess.entListIDs[i] )
				continue;

			ent->client->sess.entListIDs[i] = obj->s.number;
			Q_strncpyz( (char*)ent->client->sess.entListNames[i], buf, sizeof( ent->client->sess.entListNames[i] ) );
			break;
		}
	}
	//The appropriate spawn function will take care of
	//any ent->think() ent->die() etc function pointers, so we don't have to worry.
	return;
}

//Thanks to Raz0r for his ent tutorial that helped with making this command.
void Cmd_RemoveEntity_F( gentity_t *ent )
{
	//trace_t   tr;
	//vec3_t   fPos;
	//vec3_t   mins;
	//vec3_t   maxs;
	//int      i;
	//int      ents[8];
	int entID;
	char entIDTemp[MAX_STRING_CHARS];

	if(!G_CheckAdmin(ent, ADMIN_BUILD))
	{  	
		trap_SendServerCommand(ent-g_entities, va("print \"^1You are not allowed to use this command.\n\""));	  	
		return;  	
	}

	if ( trap_Argc() < 2 )
	{
		trap_SendServerCommand( ent-g_entities, "print \"Command Usage: /amRemoveEnt <entID>\n\"" );
		return;
		/*
		//Trace to where we're looking
		AngleVectors( ent->client->ps.viewangles, fPos, 0, 0 );
		for (i=0; i<3; i++)
		{
			fPos[i] = ent->client->ps.origin[i] + fPos[i]*Q3_INFINITE;
		}
		trap_Trace( &tr, ent->client->ps.origin, 0, 0, fPos, ent->s.number, ent->clipmask );

		//Create a box
		VectorSet( mins, tr.endpos[0]-64.0f, tr.endpos[1]-64.0f, tr.endpos[2]-64.0f );
		VectorSet( maxs, tr.endpos[0]+64.0f, tr.endpos[1]+64.0f, tr.endpos[2]+64.0f );
		trap_EntitiesInBox( mins, maxs, &ents[0], 8 );

		//Anything in this box will be removed
		for (i=0; i<8; i++)
		{
			if ( ents[i] > 0 && ents[i] < 1024 && &g_entities[ents[i]].inuse )
			{
				G_FreeEntity( &g_entities[ents[i]] ); // G_FreeEntity will free up the slot in g_entities so it can be re-used!
			}
			else
			{
				trap_SendServerCommand( ent-g_entities, "print \"^1Invalid or no ent(s) found.\n\"" );
				return;
			}
		}
		trap_SendServerCommand( ent-g_entities, "print \"^2Ent(s) you were looking at has(have) been removed.\n\"" );
		return;
		*/
	}

	trap_Argv( 1, entIDTemp, MAX_STRING_CHARS );
	entID = atoi( entIDTemp );

	if ( entID > 31 && entID < 1024 && &g_entities[entID].inuse ) //Make sure the ent isn't a player and is an ID that is in use.
	{
		G_FreeEntity( &g_entities[entID] ); // G_FreeEntity will free up the slot in g_entities so it can be re-used!
		trap_SendServerCommand( ent-g_entities, va( "print \"^2Entity with ID: ^7%i ^2removed.\n\"", entID ) );
	}
	else
	{
		trap_SendServerCommand( ent-g_entities, va( "print \"^2Invalid entID.\"", entID ) );
	}
	return;
}

void Cmd_ListEnts_F( gentity_t *ent )
{
	int i = 0;

	if(!G_CheckAdmin(ent, ADMIN_BUILD))
	{  	
		trap_SendServerCommand(ent-g_entities, va("print \"^1You are not allowed to use this command.\n\""));	  	
		return;  	
	}

	trap_SendServerCommand( ent-g_entities, "print \"^2Your entities:\n\"" );
	for ( i = 0; i < 127; i++ )
	{
		if ( !ent->client->sess.entListIDs[i] )
			continue;

		trap_SendServerCommand( ent-g_entities, va( "print \"^2# %i Name: %s\n\"", ent->client->sess.entListIDs[i], ent->client->sess.entListNames[i] ) );
	}
	return;
}

void Cmd_Invisible_F( gentity_t *ent )
{
	if(!G_CheckAdmin(ent, ADMIN_INVISIBLE))
	{  	
		trap_SendServerCommand(ent-g_entities, va("print \"^1You are not allowed to use this command.\n\""));	  	
		return;  	
	}

	   if ( ent->client->sess.isInvisible )
        {
                ent->client->sess.isInvisible = qfalse;
                ent->r.contents = CONTENTS_SOLID;
                ent->clipmask = CONTENTS_SOLID|CONTENTS_BODY;
                ent->s.trickedentindex = 0; ent->s.trickedentindex2 = 0; //This is entirely for client-side prediction. Please fix me.
                trap_SendServerCommand( ent-g_entities, "print \"^2You are no longer invisible.\n\"" );
        }
        else
        {
                ent->client->sess.isInvisible = qtrue;
                ent->r.contents = CONTENTS_BODY;
                ent->clipmask = 267009/*CONTENTS_SOLID*/;
                {//This is *entirely* for client-side prediction. Do not rely on it. Please fix me.
                        ent->client->ps.fd.forceMindtrickTargetIndex = ~(1<<ent-g_entities);
                        ent->client->ps.fd.forceMindtrickTargetIndex2 = ~(1<<ent-g_entities);
                }
                trap_SendServerCommand( ent-g_entities, "print \"^2You are now invisible.\n\"" );
        }
	   return;
}

void Cmd_Disguise_F( gentity_t *ent )
{
	if ( !ent->client->sess.isAdmin )
	{
		trap_SendServerCommand( ent-g_entities, va( "print \"^1You are not allowed to use this command.\n\"" ) );
		return;
	}

	if ( !ent->client->sess.isDisguised )
	{
		ent->client->sess.isDisguised = qtrue;
		trap_SendServerCommand( ent-g_entities, "print \"^2You are now ^7disguised\n\"" );
	}
	else
	{
		ent->client->sess.isDisguised = qfalse;
		trap_SendServerCommand( ent-g_entities, "print \"^2You are now ^7no longer disguised\n\"" );
	}
	return;
}

void Cmd_CheckStats_F( gentity_t *ent )
{
	char cmdTarget[MAX_STRING_CHARS];
	int clientid = -1; 
	int i, weapLevel;
	extern stringID_table_t FPTable[];
	extern stringID_table_t WPTable[];
	

	if(!G_CheckAdmin(ent, ADMIN_CHECKSTATS))
	{  	
		trap_SendServerCommand(ent-g_entities, va("print \"^1You are not allowed to use this command.\n\""));	  	
		return;  	
	}

	if(trap_Argc() < 2)
	{
		trap_SendServerCommand(ent-g_entities, va("print \"^2Command Usage: /amcheckstats <name/clientid>\n\""));
		return;
	}

	trap_Argv(1, cmdTarget, MAX_STRING_CHARS);

	clientid = M_G_ClientNumberFromName( cmdTarget );
	if (clientid == -1) 
	{ 
		trap_SendServerCommand( ent-g_entities, va("print \"Can't find client ID for %s\n\"", cmdTarget ) ); 
		return; 
	} 
	if (clientid == -2) 
	{ 
		trap_SendServerCommand( ent-g_entities, va("print \"Ambiguous client ID for %s\n\"", cmdTarget ) ); 
		return; 
	}
	if (clientid >= MAX_CLIENTS || clientid < 0) 
	{ 
		trap_SendServerCommand( ent-g_entities, va("print \"Bad client ID for %s\n", cmdTarget ) );
		return;
	}
	if (!g_entities[clientid].inuse) 
	{
		trap_SendServerCommand( ent-g_entities, va("print \"Client %s is not active\n\"", cmdTarget ) ); 
		return; 
	}

	trap_SendServerCommand( ent-g_entities, va( "print \"^2Force Stats for ^7%s^2:\n\"", g_entities[clientid].client->pers.netname ) );
	//It's NUM_FORCE_POWERS-1 because force powers go from 0-17, not 1-18
	for ( i = 0; i < NUM_FORCE_POWERS-1; i++ )
	{	
		if ( FPTable[i].id != -1 && FPTable[i].name[0] )
		{
			trap_SendServerCommand( ent-g_entities, va( "print \"^7%s ^2- Level: ^7%i\n\"", FPTable[i].name, g_entities[clientid].client->ps.fd.forcePowerLevel[i] ) );
		}
	}
	for ( i = 1; i < WP_NUM_WEAPONS; i++ )
	{	
		if ( i == 2 || i == 3 || i == 16 || i == 17 || i == 18 )
			continue;
		//OPENRP TODO - Saber style, specific grenade type
		if ( WPTable[i].id != -1 && WPTable[i].name[0] )
		{
			switch ( i )
			{
			case 1:
				weapLevel = 24;
				break;
			case 4:
				weapLevel = 1;
				break;
			case 5:
				weapLevel = 2;
				break;
			case 6:
				weapLevel = 14;
				break;
			case 7:
				weapLevel = 7;
				break;
			case 8:
				weapLevel = 13;
				break;
			case 9:
				weapLevel = 31;
				break;
			case 10:
				weapLevel = 22;
				break;
			case 11:
				weapLevel = 4;
				break;
			case 12:
				weapLevel = 3;
				break;
			case 13:
				weapLevel = 26;
				break;
			case 14:
				weapLevel = 12;
				break;
			case 15:
				weapLevel = 32;
				break;
			}
			trap_SendServerCommand( ent-g_entities, va( "print \"^7%s ^2- Level: ^7%i\n\"", WPTable[i].name, g_entities[clientid].client->skillLevel[weapLevel] ) );
		}
	}
	return;
}

void Cmd_ToBlack_F( gentity_t *ent )
{
	qboolean fadeToBlack, toBlackImmediately;
	char parameter[7];

	if( !G_CheckAdmin( ent, ADMIN_TOBLACK ) )
	{
		trap_SendServerCommand(ent-g_entities, va("print \"^1You are not allowed to use this command.\n\""));
		return;
	}
	if ( trap_Argc() < 2 )
	{
		trap_SendServerCommand( ent-g_entities, "print \"^2Command Usage: /amToBlack <fade/nofade> Fading takes 10 seconds.\n\"" );
		return;
	}

	trap_Argv( 1, parameter, sizeof( parameter ) );

	if( !Q_stricmpn( parameter, "fade", 4 ) )
	{

		if ( !ent->client->sess.fadeToBlack )
			fadeToBlack = qtrue;
		else
			fadeToBlack = qfalse;

		if ( fadeToBlack )
			trap_SendServerCommand( ent-g_entities, "print \"^2All players' screens now fade to black...\n\"" );
		else
			trap_SendServerCommand( ent-g_entities, "print \"^2All players' screens now come back to normal.\n\"" );

		ent->client->sess.fadeToBlack = fadeToBlack;

		trap_SendServerCommand( -1, "fadeToBlack" );
	}
	else if( !Q_stricmpn( parameter, "nofade", 6 ) )
	{
		if ( !ent->client->sess.toBlackImmediately )
			toBlackImmediately = qtrue;
		else
			toBlackImmediately = qfalse;

		if ( toBlackImmediately )
			trap_SendServerCommand( ent-g_entities, "print \"^2All players' screens now immediately turn black...\n\"" );
		else
			trap_SendServerCommand( ent-g_entities, "print \"^2All players screens now come back to normal.\n\"" );

		ent->client->sess.toBlackImmediately = toBlackImmediately;

		trap_SendServerCommand( -1, "toBlackImmediately" );
	}
	else
	{
		trap_SendServerCommand( ent-g_entities, "print \"^2Command Usage: /amToBlack <fade/nofade> Fading takes 10 seconds.\n\"" );
		return;
	}

	return;
}

void Cmd_Timer_F( gentity_t *ent )
{
	char secondsTemp[10], color[10];
	int seconds;
	qboolean isMyTeam;

	if( !G_CheckAdmin( ent, ADMIN_TIMER ) )
	{
		trap_SendServerCommand(ent-g_entities, va("print \"^1You are not allowed to use this command.\n\""));
		return;
	}

	if ( trap_Argc() != 3 )
	{
		trap_SendServerCommand(ent-g_entities, "print \"^2Command Usage: /amTimer <seconds> <red/green>\nRed or green determines the color of the numbers displayed.\n\"" );
		return;
	}

	trap_Argv( 1, secondsTemp, sizeof ( secondsTemp ) );
	seconds = atoi( secondsTemp );
	trap_Argv( 2, color, sizeof( color ) );

	if ( !Q_stricmp( color, "red" ) )
	{
		isMyTeam = qtrue;
	}
	else if ( !Q_stricmp( color, "green" ) )
	{
		isMyTeam = qfalse;
	}
	else
	{
		trap_SendServerCommand(ent-g_entities, "print \"^2Command Usage: /amTimer <seconds> <red/green>\nRed or green determines the color of the numbers displayed.\n\"" );
		return;
	}

	trap_SendServerCommand( -1, va( "timer %i %i", seconds, isMyTeam ) );
	trap_SendServerCommand( ent-g_entities, va( "print \"^2Timer colored ^7%s ^2with ^7%i ^2seconds started.\n\"", color, seconds ) );
	return;
}
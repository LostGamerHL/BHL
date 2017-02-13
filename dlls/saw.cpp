/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "soundent.h"
#include "gamerules.h"
#define WEAPON_SAW 16


class CSaw : public CBasePlayerWeapon
{
public:

#ifndef CLIENT_DLL
	int		Save(CSave &save);
	int		Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];
#endif

	void Spawn(void);
	void Precache(void);
	int iItemSlot(void) { return 6; }
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer(CBasePlayer *pPlayer);

	void PrimaryAttack(void);
	BOOL Deploy(void);
	void Reload(void);
	void WeaponIdle(void);
	virtual BOOL ShouldWeaponIdle(void) { return TRUE; }
	float m_flNextAnimTime;
	int m_iShell;

	virtual BOOL UseDecrement(void)
	{
		return FALSE;
	}

	void ReloadStart( void );
	void ReloadInsert( void );

	enum M249_RELOAD_STATE { RELOAD_STATE_NONE = 0, RELOAD_STATE_OPEN, RELOAD_STATE_FILL };

	int m_iReloadState;

private:
	//unsigned short m_usM249;
};

TYPEDESCRIPTION CSaw::m_SaveData[] =
{
	DEFINE_FIELD(CSaw, m_iReloadState, FIELD_INTEGER),
};
IMPLEMENT_SAVERESTORE(CSaw, CBasePlayerWeapon);


enum m249_e
{
	M249_SLOWIDLE = 0,
	M249_IDLE2,
	M249_LAUNCH,
	M249_RELOAD1,
	M249_HOLSTER,
	M249_DEPLOY,
	M249_SHOOT1,
	M249_SHOOT2,
	M249_SHOOT3,
};


LINK_ENTITY_TO_CLASS(weapon_saw, CSaw);


//=========================================================
//=========================================================

void CSaw::Spawn()
{
	Precache();
	SET_MODEL(ENT(pev), "models/w_saw.mdl");
	m_iId = WEAPON_SAW;

	m_iDefaultAmmo = 150;

	m_iReloadState = RELOAD_STATE_NONE;

	FallInit();// get ready to fall down.
}


void CSaw::Precache(void)
{
	PRECACHE_MODEL("models/v_saw.mdl");
	PRECACHE_MODEL("models/w_saw.mdl");
	PRECACHE_MODEL("models/p_saw.mdl");

	m_iShell = PRECACHE_MODEL("models/saw_shell.mdl");// brass shellTE_MODEL

	PRECACHE_MODEL("models/w_saw_clip.mdl");
	PRECACHE_SOUND("items/9mmclip1.wav");

	PRECACHE_SOUND("weapons/saw_fire1.wav");
	PRECACHE_SOUND("weapons/saw_fire2.wav");
	PRECACHE_SOUND("weapons/saw_fire3.wav");

	PRECACHE_SOUND("weapons/saw_reload.wav");
	PRECACHE_SOUND("weapons/saw_reload2.wav");

	PRECACHE_SOUND("weapons/357_cock1.wav");

	//m_usM249 = PRECACHE_EVENT(1, "events/m249.sc");
}

int CSaw::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "556";
	p->iMaxAmmo1 = 500;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = 100;
	p->iSlot = 5;
	p->iPosition = 0;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_SAW;
	p->iWeight = 10;

	return 1;
}

int CSaw::AddToPlayer(CBasePlayer *pPlayer)
{
	if (CBasePlayerWeapon::AddToPlayer(pPlayer))
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev);
			WRITE_BYTE(m_iId);
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}

BOOL CSaw::Deploy()
{
	return DefaultDeploy("models/v_saw.mdl", "models/p_saw.mdl", M249_DEPLOY, "m249");
}


void CSaw::PrimaryAttack()
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = gpGlobals->time + 0.15;
		return;
	}

	if (m_iClip <= 0)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack =gpGlobals->time + 0.15;
		return;
	}

	m_pPlayer->pev->punchangle.x = RANDOM_FLOAT( 1.0f, 1.5f );
	m_pPlayer->pev->punchangle.y = RANDOM_FLOAT( -0.5f, -0.2f );

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_iClip--;


	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);
	Vector vecDir;

#ifdef CLIENT_DLL
	if (!bIsMultiplayer())
#else
	if (!g_pGameRules->IsMultiplayer())
#endif
	{
		// optimized multiplayer. Widened to make it easier to hit a moving player
		vecDir = m_pPlayer->FireBulletsPlayer(1, vecSrc, vecAiming, VECTOR_CONE_6DEGREES, 8192, BULLET_PLAYER_556, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed);
	}
	else
	{
		// single player spread
		vecDir = m_pPlayer->FireBulletsPlayer(1, vecSrc, vecAiming, VECTOR_CONE_3DEGREES, 8192, BULLET_PLAYER_556, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed);
	}

	int flags;
	flags = FEV_NOTHOST;
		switch( RANDOM_LONG( 0, 2 ) )
		{
		case 0:
		SendWeaponAnim( M249_SHOOT1 );
			break;
		case 1:
		SendWeaponAnim( M249_SHOOT2 );
			break;
		case 2:
		SendWeaponAnim( M249_SHOOT3 );
			break;
}
	pev->effects |= EF_MUZZLEFLASH;
	//PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usM249, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0);


#ifndef CLIENT_DLL
	// Add inverse impulse, but only if we are on the ground.
	// This is mainly to avoid too-high air velocity.
	if (m_pPlayer->pev->flags & FL_ONGROUND)
	{
		float flZVel = m_pPlayer->pev->velocity.z;

		m_pPlayer->pev->velocity = m_pPlayer->pev->velocity - gpGlobals->v_forward * (40 + (RANDOM_LONG(1, 2) * 2));

		// Add backward velocity
		m_pPlayer->pev->velocity.z = flZVel;
	}
#endif

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flNextPrimaryAttack = gpGlobals->time +0.1;

	if (m_flNextPrimaryAttack < UTIL_WeaponTimeBase())
		m_flNextPrimaryAttack = gpGlobals->time + UTIL_WeaponTimeBase() + 0.1;

	m_flTimeWeaponIdle = gpGlobals->time + UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
}


void CSaw::Reload(void)
{
	if (m_pPlayer->ammo_556 <= 0)
		return;

	DefaultReload(100, M249_RELOAD1, 1.5);
}


void CSaw::WeaponIdle(void)
{
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	int iAnim;
	switch (RANDOM_LONG(0, 1))
	{
	case 0:
		iAnim = M249_SLOWIDLE;
		break;

	default:
	case 1:
		iAnim = M249_IDLE2;
		break;
	}

	SendWeaponAnim(iAnim);

	m_flTimeWeaponIdle = gpGlobals->time + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15); // how long till we do this again.
}


void CSaw::ReloadStart(void)
{
	SendWeaponAnim(M249_RELOAD1, UseDecrement());
}

void CSaw::ReloadInsert(void)
{
	SendWeaponAnim(M249_RELOAD1, UseDecrement());
}

class CSawAmmoClip : public CBasePlayerAmmo
{
	void Spawn(void)
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_saw_clip.mdl");
		CBasePlayerAmmo::Spawn();
	}
	void Precache(void)
	{
		PRECACHE_MODEL("models/w_saw_clip.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo(CBaseEntity *pOther)
	{
		int bResult = (pOther->GiveAmmo(150, "556", 500) != -1);
		if (bResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};

LINK_ENTITY_TO_CLASS(ammo_556, CSawAmmoClip);

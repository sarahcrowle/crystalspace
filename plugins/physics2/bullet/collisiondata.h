/*
    Copyright (C) 2013 Christian Van Brussel, Institute of Information
      and Communication Technologies, Electronics and Applied Mathematics
      at Universite catholique de Louvain, Belgium
      http://www.uclouvain.be/en-icteam.html

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
#ifndef __CS_BULLET_COLLISIONDATA_H__
#define __CS_BULLET_COLLISIONDATA_H__

#include "csutil/scf_implementation.h"

CS_PLUGIN_NAMESPACE_BEGIN (Bullet2)
{

class CollisionContact
  : public scfImplementation1<CollisionContact,
  CS::Collisions::iCollisionContact>
{
public:
  csVector3 positionOnA;
  csVector3 positionOnB;
  csVector3 normalOnB;
  float penetration;

public:
  CollisionContact ()
    : scfImplementationType (this) {}

  virtual csVector3 GetPositionOnA () const
  { return positionOnA; }
  virtual csVector3 GetPositionOnB () const
  { return positionOnB; }
  //virtual csVector3 GetNormalOnA () const
  virtual csVector3 GetNormalOnB () const
  { return normalOnB; }
  virtual float GetPenetration () const
  { return penetration; }
};

class CollisionData
  : public scfImplementation1<CollisionData,
  CS::Collisions::iCollisionData>
{
public:
  CS::Collisions::iCollisionObject* objectA;
  CS::Collisions::iCollisionObject* objectB;
  csRefArray<CollisionContact> contacts;

public:
  CollisionData ()
    : scfImplementationType (this), objectA (nullptr), objectB (nullptr)
  {}

  virtual CS::Collisions::iCollisionObject* GetObjectA () const
  { return objectA; }
  virtual CS::Collisions::iCollisionObject* GetObjectB () const
  { return objectB; }
  virtual size_t GetContactCount () const
  { return contacts.GetSize (); }
  virtual CS::Collisions::iCollisionContact* GetContact (size_t index)
  { return contacts[index]; }
};

class CollisionDataList
  : public scfImplementation1<CollisionDataList,
  CS::Collisions::iCollisionDataList>
{
public:
  csRefArray<CollisionData> collisions;

public:
  CollisionDataList ()
    : scfImplementationType (this) {}

  virtual size_t GetCollisionCount () const
  { return collisions.GetSize (); }
  virtual CS::Collisions::iCollisionData* GetCollision (size_t index) const
  { return collisions[index]; }
};

}
CS_PLUGIN_NAMESPACE_END (Bullet2)

#endif //  __CS_BULLET_COLLISIONDATA_H__

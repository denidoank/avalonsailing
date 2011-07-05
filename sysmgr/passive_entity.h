// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
#ifndef SYSMGR_PASSIVE_ENTITY_H__
#define SYSMGR_PASSIVE_ENTITY_H__

#include "sysmgr/entity.h"

class PowerBusEntity :  public Entity {
public:
  PowerBusEntity(const char *name, const char *cmd,
                 const RecoveryPolicy &policy, SysMon *sysmon)
      : cmd_(cmd), Entity(name, policy, sysmon) {};
  ~PowerBusEntity();
  void RecoveryAction();

private:
  const std::string cmd_;
};

class SystemEntity : public PowerBusEntity {
 public:
  SystemEntity(const char *name, const char *cmd,
               const RecoveryPolicy &policy, SysMon *sysmon) :
      PowerBusEntity(name, cmd, policy, sysmon) {};

  void RecoveryAction();
};

#endif // SYSMGR_PASSIVE_ENTITY_H__

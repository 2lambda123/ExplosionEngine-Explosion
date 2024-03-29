//
// Created by johnk on 2023/9/1.
//

#pragma once

#include <Core/Module.h>
#include <Runtime/Api.h>
#include <Runtime/World.h>

namespace Runtime {
    class RUNTIME_API RuntimeModule : public Core::Module {
    public:
        RuntimeModule();
        ~RuntimeModule() override;

        World* CreateWorld(const std::string& inName = "") const;
        void DestroyWorld(World* inWorld) const;
    };
}

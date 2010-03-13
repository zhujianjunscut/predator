/*
 * Copyright (C) 2009 Kamil Dudka <kdudka@redhat.com>
 *
 * This file is part of sl.
 *
 * sl is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * sl is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with sl.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "cl_symexec.hh"

#include "cl_msg.hh"
#include "cl_storage.hh"
#include "storage.hh"
#include "symexec.hh"

#include <string>

class ClSymExec: public ClStorageBuilder {
    public:
        ClSymExec(const char *configString);
        virtual ~ClSymExec();

    protected:
        virtual void run(CodeStorage::Storage &);

    private:
        std::string configString_;
};

// /////////////////////////////////////////////////////////////////////////////
// ClSymExec implementation
ClSymExec::ClSymExec(const char *configString):
    configString_(configString)
{
}

ClSymExec::~ClSymExec() {
}

namespace {
    void initExec(SymExec &se, const std::string &cnf) {
        using std::string;

        if (string("fast") == cnf) {
            CL_DEBUG("SymExec \"fast mode\" requested");
            se.setFastMode(true);
        }
    }
}

void ClSymExec::run(CodeStorage::Storage &stor) {
    CL_DEBUG("looking for 'main()' at gl scope...");
    const int uid = stor.fncNames.glNames["main"];
    CodeStorage::Fnc *main = stor.fncs[uid];
    if (!main) {
        CL_ERROR("main() not declared at global scope");
        return;
    }

    // run the symbolic execution
    SymExec se(stor);
    initExec(se, configString_);
    se.exec(*main);
}


// /////////////////////////////////////////////////////////////////////////////
// public interface, see cl_symexec.hh for more details
ICodeListener* createClSymExec(const char *configString) {
    return new ClSymExec(configString);
}

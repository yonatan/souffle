/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2013, 2014, Oracle and/or its affiliates. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file UnionType.h
 *
 * Defines the union type class
 *
 ***********************************************************************/

#pragma once

#include "ast/Node.h"
#include "ast/QualifiedName.h"
#include "ast/Type.h"
#include "parser/SrcLocation.h"
#include "souffle/utility/MiscUtil.h"
#include "souffle/utility/StreamUtil.h"
#include <algorithm>
#include <cstddef>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace souffle::ast {

/**
 * @class UnionType
 * @brief The union type class
 *
 * Example:
 *  .type A = B1 | B2 | ... | Bk
 *
 * A union type combines multiple types into a new super type.
 * Each of the enumerated types become a sub-type of the new
 * union type.
 */
class UnionType : public Type {
public:
    UnionType(QualifiedName name, std::vector<QualifiedName> types, SrcLocation loc = {})
            : Type(std::move(name), std::move(loc)), types(std::move(types)) {}

    /** Return list of unioned types */
    const std::vector<QualifiedName>& getTypes() const {
        return types;
    }

    std::vector<QualifiedName>& getTypes() {
        return types;
    }

    /** Add another unioned type */
    void add(QualifiedName type) {
        types.push_back(std::move(type));
    }

    /** Set type */
    void setType(size_t idx, QualifiedName type) {
        types.at(idx) = std::move(type);
    }

    UnionType* clone() const override {
        return new UnionType(getQualifiedName(), types, getSrcLoc());
    }

protected:
    void print(std::ostream& os) const override {
        os << ".type " << getQualifiedName() << " = " << join(types, " | ");
    }

    bool equal(const Node& node) const override {
        const auto& other = asAssert<UnionType>(node);
        return getQualifiedName() == other.getQualifiedName() && types == other.types;
    }

private:
    /** List of unioned types */
    std::vector<QualifiedName> types;
};

}  // namespace souffle::ast

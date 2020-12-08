/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2018 The Souffle Developers. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file ProvenanceClauseTranslator.h
 *
 * Clause translator when provenance is used
 *
 ***********************************************************************/

#pragma once

#include "ast2ram/ClauseTranslator.h"

namespace souffle::ast {
class Clause;
}

namespace souffle::ram {
class Condition;
class Operation;
class Statement;
}  // namespace souffle::ram

namespace souffle::ast2ram {

class TranslatorContext;

class ProvenanceClauseTranslator : public ClauseTranslator {
public:
    ProvenanceClauseTranslator(const TranslatorContext& context, SymbolTable& symbolTable)
            : ClauseTranslator(context, symbolTable) {}

    static Own<ram::Statement> generateClause(const TranslatorContext& context, SymbolTable& symbolTable,
            const ast::Clause& clause, int version = 0);

protected:
    Own<ram::Statement> createRamFactQuery(const ast::Clause& clause) const override;
    Own<ram::Statement> createRamRuleQuery(const ast::Clause& clause, size_t version) override;
    Own<ram::Operation> createValueSubroutine(const ast::Clause& clause) const;
    Own<ram::Operation> addNegate(const ast::Atom* atom, Own<ram::Operation> op, bool isDelta) const override;
};
}  // namespace souffle::ast2ram

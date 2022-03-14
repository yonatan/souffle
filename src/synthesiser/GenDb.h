/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2018, The Souffle Developers. All rights reserved.
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file GenDb.h
 *
 * Provides some classes to represent C++ classes, functions, etc.
 * This permits as a little more structured way to generate the C++ code
 * instead of only relying on std::ostream in the Synthesiser.
 ***********************************************************************/

#pragma once

#include "souffle/utility/Types.h"
#include <cassert>
#include <filesystem>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <ostream>
#include <set>
#include <sstream>
#include <vector>

namespace fs = std::filesystem;

namespace souffle::synthesiser {

/** An output stream where some pieces may be filled later or conditionnaly. */
class DelayableOutputStream : public std::streambuf, public std::ostream {
public:
    DelayableOutputStream() : std::ostream(this) {}

    ~DelayableOutputStream() {}

    std::streambuf::int_type overflow(std::streambuf::int_type ch) override;

    /** Return a piece of stream that will be included in the output only if the given condition is true when
     * this stream is flushed. */
    std::shared_ptr<std::ostream> delayed_if(const bool& cond);

    /** Return a piece of stream that will be included in the output when this stream is flushed. */
    std::shared_ptr<std::ostream> delayed();

    /** */
    void flushAll(std::ostream& os);

private:
    /* the sequence of pieces that compose the output stream. */
    std::list<std::pair<std::optional<const bool*>, std::shared_ptr<std::stringstream>>> pieces;

    /* points to the current piece's stream. */
    std::shared_ptr<std::ostream> current_stream;
};

/**
 * Object representing some C++ construct that should be emitted in the
 * generated code. For instance, a class, a function...
 */
class Gen {
public:
    Gen(std::string name) : name(name) {}

    /* Emit the declaration of this construct in C++,
     * typically what we would expect from a .hpp file
     */
    virtual void declaration(std::ostream& o) = 0;

    /* Emit the Implementation of this construct in C++,
     * typically what we would expect from a .cpp file
     */
    virtual void definition(std::ostream& o) = 0;

    std::string& getName() {
        return name;
    }

    /*
     * Basename of the file (without extension) where
     * that should be produced for this code.
     */
    virtual fs::path fileBaseName() {
        return fs::path(name);
    };

    /*
     * Sets 'dep' as a class that is used by the current construct
     * and must be #include in the current construct
     * Set 'def_only' to true if only the .cpp file must include it.
     */
    void addDependency(Gen& dep, bool def_only = false);

    /*
     * #include 'str' must be included when generating the code
     */
    void addInclude(std::string str, bool def_only = false);

    /**
     * Accessors for private members
     */
    std::set<std::string>& getDeclIncludes() {
        return decl_includes;
    }
    std::set<std::string>& getIncludes() {
        return includes;
    }
    std::set<Gen*>& getDeclDependencies() {
        return decl_dependencies;
    }
    std::set<Gen*>& getDependencies() {
        return dependencies;
    }

    fs::path getHeader() {
        return fileBaseName().concat(".hpp");
    }

protected:
    std::string name;

    std::set<std::string> decl_includes;
    std::set<Gen*> decl_dependencies;
    std::set<std::string> includes;
    std::set<Gen*> dependencies;
};

class GenClass;
class GenDb;

/**
 * Visibility of the elements in the class
 */
enum Visibility { Public = 0, Private };

/**
 * Class helper to manipulate/build a function to be emitted
 * by the C++ Synthesizer.
 */
class GenFunction : public Gen {
public:
    GenFunction(std::string name, GenClass* cl, Visibility v)
            : Gen(name), cl(cl), visibility(v), override(false) {}

    void setRetType(std::string ty);
    void setNextArg(std::string ty, std::string name, std::optional<std::string> defaultValue = std::nullopt);
    void setNextInitializer(std::string name, std::string value);

    void setIsConstructor() {
        isConstructor = true;
    };
    void setOverride() {
        override = true;
    };

    void declaration(std::ostream& o) override;

    void definition(std::ostream& o) override;

    Visibility getVisibility() {
        return visibility;
    }

    std::ostream& body() {
        return bodyStream;
    }

private:
    GenClass* cl;
    Visibility visibility;
    bool isConstructor = false;
    bool override;
    std::string retType;
    std::vector<std::tuple<std::string, std::string, std::optional<std::string>>> args;
    std::vector<std::pair<std::string, std::string>> initializer;
    std::stringstream bodyStream;
};

/**
 * Class helper to manipulate/build a class to be emitted
 * by the C++ Synthesizer.
 */
class GenClass : public Gen {
public:
    GenClass(std::string name, GenDb* db) : Gen(name), db(db) {}
    GenFunction& addFunction(std::string name, Visibility);
    GenFunction& addConstructor(Visibility);

    void addField(
            std::string type, std::string name, Visibility, std::optional<std::string> init = std::nullopt);

    void declaration(std::ostream& o) override;

    void definition(std::ostream& o) override;

    void inherits(std::string parent) {
        inheritance.push_back(parent);
    }

    bool ignoreUnusedArgumentWarning = false;
    bool isMain = false;

private:
    GenDb* db;
    std::vector<Own<GenFunction>> methods;
    std::vector<std::tuple<std::string /*name*/, std::string /*type*/, Visibility,
            std::optional<std::string> /* initializer value */
            >>
            fields;
    std::vector<std::string> inheritance;
};

/**
 * Class helper to manipulate/build the class
 * for one of the Souffle specialized datastructures
 * (e.g. BTree, BTreeDelete, Brie, etc.)
 */
class GenDatastructure : public Gen {
public:
    GenDatastructure(std::string name, std::optional<std::string> namespace_opt, GenDb* db)
            : Gen(name), namespace_name(namespace_opt), db(db) {}

    std::ostream& decl() {
        return declarationStream;
    }
    std::ostream& def() {
        return definitionStream;
    }

    void declaration(std::ostream& o) override;
    void definition(std::ostream& o) override;

    fs::path fileBaseName() override;

private:
    std::optional<std::string> namespace_name;
    GenDb* db;
    std::stringstream declarationStream;
    std::stringstream definitionStream;
};

/**
 * Class that stores all the constructs build by the Synthesizer.
 * Provides methods to emit the generated C++ code to a unique
 * or to multiple files.
 */
class GenDb {
public:
    GenClass& getClass(std::string name);
    GenDatastructure& getDatastructure(std::string name, std::optional<std::string> namespace_opt);

    void emitSingleFile(std::ostream& o);
    void emitMultipleFilesInDir(std::string dir);

    std::ostream& hooks() {
        return hiddenHooksStream;
    }
    std::ostream& externC() {
        return externCStream;
    }

    void addGlobalInclude(std::string str) {
        globalIncludes.emplace(str);
    }
    void addGlobalDefine(std::string str) {
        globalDefines.emplace(str);
    }

    void usesDatastructure(GenClass& cl, std::string str) {
        if (nameToGen.count(str)) {
            cl.addDependency(*nameToGen[str]);
        } else if (nameToInclude.count(str)) {
            cl.addInclude(nameToInclude[str]);
        }
    }

    void datastructureIncludes(std::string datastructure, std::string inc) {
        nameToInclude[datastructure] = inc;
    }

private:
    std::vector<Own<GenDatastructure>> datastructures;
    std::vector<Own<GenClass>> classes;

    std::map<std::string, Gen*> nameToGen;
    std::map<std::string, std::string> nameToInclude;

    std::stringstream hiddenHooksStream, externCStream;

    std::set<std::string> globalIncludes;
    std::set<std::string> globalDefines;
};

}  // namespace souffle::synthesiser
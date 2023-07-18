/// <reference path="types/native.d.ts"/>
import url from "node:url";
import path from "node:path";

import { createRequire } from "node:module";
const require = createRequire(import.meta.url);
const homePath = path.join(url.fileURLToPath(new URL(".", import.meta.url)), "/../..");
const modulePath = path.join(homePath, `../build/libretro-x64-Release-LLVM`);
const {
	MImm,
	MReg,
	MMem,
	MInsn,
	Z3Expr,
	Z3VariableSet,
	Const,
	Operand,
	Value,
	Insn,
	BasicBlock,
	Routine,
	Arch,
	Loader,
	Scheduler,
	Task,
	Image,
	Workspace,
	Clang,
} = require(modulePath) as typeof LibRetro;
export type MImm = LibRetro.MImm;
export type MReg = LibRetro.MReg;
export type MMem = LibRetro.MMem;
export type MInsn = LibRetro.MInsn;
export type Z3Expr = LibRetro.Z3Expr;
export type Z3VariableSet = LibRetro.Z3VariableSet;
export type Const = LibRetro.Const;
export type Operand = LibRetro.Operand;
export type Value = LibRetro.Value;
export type Insn = LibRetro.Insn;
export type BasicBlock = LibRetro.BasicBlock;
export type Routine = LibRetro.Routine;
export type Arch = LibRetro.Arch;
export type Loader = LibRetro.Loader;
export type Scheduler = LibRetro.Scheduler;
export type Task<T> = LibRetro.Task<T>;
export type Image = LibRetro.Image;
export type Workspace = LibRetro.Workspace;

export {
	MImm,
	MReg,
	MMem,
	MInsn,
	Z3Expr,
	Z3VariableSet,
	Const,
	Operand,
	Value,
	Insn,
	BasicBlock,
	Routine,
	Arch,
	Loader,
	Scheduler,
	Task,
	Image,
	Workspace,
	Clang,
};

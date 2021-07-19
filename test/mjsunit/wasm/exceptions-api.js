// Copyright 2021 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Flags: --experimental-wasm-eh

load("test/mjsunit/wasm/wasm-module-builder.js");

(function TestImport() {
  print(arguments.callee.name);

  assertThrows(() => new WebAssembly.Tag(), TypeError,
      /Argument 0 must be a tag type/);
  assertThrows(() => new WebAssembly.Tag({}), TypeError,
      /Argument 0 must be a tag type with 'parameters'/);
  assertThrows(() => new WebAssembly.Tag({parameters: ['foo']}), TypeError,
      /Argument 0 parameter type at index #0 must be a value type/);
  assertThrows(() => new WebAssembly.Tag({parameters: {}}), TypeError,
      /Argument 0 contains parameters without 'length'/);

  let js_except_i32 = new WebAssembly.Tag({parameters: ['i32']});
  let js_except_v = new WebAssembly.Tag({parameters: []});
  let builder = new WasmModuleBuilder();
  builder.addImportedException("m", "ex", kSig_v_i);

  assertDoesNotThrow(() => builder.instantiate({ m: { ex: js_except_i32 }}));
  assertThrows(
      () => builder.instantiate({ m: { ex: js_except_v }}), WebAssembly.LinkError,
      /imported tag does not match the expected type/);
  assertThrows(
      () => builder.instantiate({ m: { ex: js_except_v }}), WebAssembly.LinkError,
      /imported tag does not match the expected type/);
})();

(function TestExport() {
  print(arguments.callee.name);
  let builder = new WasmModuleBuilder();
  let except = builder.addException(kSig_v_v);
  builder.addExportOfKind("ex", kExternalException, except);
  let instance = builder.instantiate();

  assertTrue(Object.prototype.hasOwnProperty.call(instance.exports, 'ex'));
  assertEquals("object", typeof instance.exports.ex);
  assertInstanceof(instance.exports.ex, WebAssembly.Tag);
  assertSame(instance.exports.ex.constructor, WebAssembly.Tag);
})();

(function TestImportExport() {
  print(arguments.callee.name);

  let js_ex_i32 = new WebAssembly.Tag({parameters: ['i32']});
  let builder = new WasmModuleBuilder();
  let index = builder.addImportedException("m", "ex", kSig_v_i);
  builder.addExportOfKind("ex", kExternalException, index);

  let instance = builder.instantiate({ m: { ex: js_ex_i32 }});
  let res = instance.exports.ex;
  assertEquals(res, js_ex_i32);
})();

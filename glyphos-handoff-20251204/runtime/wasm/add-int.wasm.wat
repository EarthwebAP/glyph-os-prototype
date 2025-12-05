;; add-int.wasm - Simple WASM module for testing
;; Compile: wat2wasm add-int.wasm.wat -o add-int.wasm

(module
  (func (export "add") (param $a i32) (param $b i32) (result i32)
    local.get $a
    local.get $b
    i32.add
  )
)

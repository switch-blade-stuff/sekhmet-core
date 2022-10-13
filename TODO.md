### TODO:

- [ ] Remove the `read`/`write` API for serialization, instead use `get`/`set`/`as`.
- [ ] Refactor type name generation to strip out cv-qualifiers, struct, class & union keywords and stray spaces. This
  would allow for cross-compiler type names.
- [ ] Inject type factory as an optional argument to in-place constructed attributes, to allow the attribute attach
  dependency metadata.
- [ ] Make the top-level CMakeLists suitable for including as a library.
- [ ] Rewrite tests.
- [x] Make math structures use packed format by default, for ABI compatibility.
- [ ] Use git submodules for third party dependencies.

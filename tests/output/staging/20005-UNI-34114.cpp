//TestCase-001
///      |
/// +---------+       \
/// |         |       |
/// |         |       |
/// |  Plane  | --> X | "height"
/// |         |       |
/// |         |       |
/// +---------+       /
/// \- width -/

//TestCase-002
//      |
// +---------+       \
// |         |       |
// |         |       |
// |  Plane  | --> X | "height"
// |         |       |
// |         |       |
// +---------+       /
// \- width -/

//TestCase-003
/*      |
/ +---------+       \
/ |         |       |
/ |         |       |
/ |  Plane  | --> X | "height"
/ |         |       |
/ |         |       |
/ +---------+       /
/ \- width -/ */

/// E.g. you might want have a single "editor_assets:\" root which combines multiple Assets folders together:
/// project's "Assets", global shared (tools) "Assets", collab assets, etc.
/// editor_assets:\
/// |-project_assets\ - ".\Assets"
/// |-shared_assets\  - "C:\MyStudio\Assets"
/// |-builtins_assets\  - "%programfiles%\Unity\BuiltinAssets"
/// | ...

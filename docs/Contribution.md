# WeatherMaestro Code Contribution rules

## Formatting
1. TABS = 2 spaces
2. Pointer asterisk on left when declaring
- Example: `int* ptr`
- On the right for dereferencing only: `*ptr`

## Naming conventions

### General variables
- lowercase 
- snake_case
- Begin with underscore when passed in as function argument

Example:
`int foo_bar;`
`int sum_func(int _foo_bar);`

### Structs
- Capital
- Snake_Case

Example:
`This_Is_A_Struct_Name Struct_Var`

### Enums
- Capital
- No underscore
- INTEGER CONTENTS IN BIG CAPS

Example:
`enum ExampleEnum {
  FIRST_ENUM  = 0,
  SECOND_ENUM = 1
}`


# Грамматика языка

```
Program                 ::= FunctionDeclaration* ;

FunctionDeclaration     ::= "func" Identificator "(" ParameterList? ")" (":" Type)? Scope ;

ParameterList           ::= Parameter ("," Parameter)* ;
Parameter               ::= Identificator ":" Type ;

Type                    ::= BuiltinType | Identificator ;

BuiltinType             ::= "int" | "float" | "string" | "bool" ;

Scope                   ::= "{" Statement* "}" ;

Statement               ::= ReturnStatement
                          | ExpressionStatement
                          | IfStatement
                          | WhileStatement
                          | VariableDeclaration
                          | Scope ;

ReturnStatement         ::= "ret" Expression? ;

ExpressionStatement     ::= Expression ;

IfStatement             ::= "if" "(" Expression ")" Scope ("else" Scope)? ;

WhileStatement          ::= "while" "(" Expression ")" Scope ;

VariableDeclaration     ::= "var" Identificator ":" Type ;

Expression              ::= AssignmentExpression ;

AssignmentExpression    ::= LogicalOrExpression ( "<-" AssignmentExpression )? ;

LogicalOrExpression     ::= LogicalAndExpression ( ("or" | "xor") LogicalAndExpression )* ;

LogicalAndExpression    ::= EqualityExpression ( "and" EqualityExpression )* ;

EqualityExpression      ::= Comparison ( ("==" | "!=") Comparison )* ;

Comparison              ::= Term ( ("<" | ">" | "<=" | ">=") Term )* ;

Term                    ::= Factor ( ("-" | "+") Factor )* ;

Factor                  ::= Unary ( ("*" | "/" | "%") Unary )* ;

Unary                   ::= ("not" | "+" | "-") Unary
                          | Postfix ;

Postfix                 ::= Primary ( Call )* ;

Call                    ::= "(" ( Expression ("," Expression)* )? ")" ;

Primary                 ::= Literal
                          | Identificator
                          | "(" Expression ")" ;

Literal                 ::= IntLiteral
                          | FloatLiteral
                          | StrLiteral ;
```
# Грамматика языка

```
Program                 ::= FunctionDeclaration* ;

FunctionDeclaration     ::= "func" Identificator "(" ParameterList? ")" Scope ;

ParameterList           ::= Parameter ("," Parameter)* ;
Parameter               ::= Identificator ;

Scope                   ::= "{" Statement* "}" ;

Statement               ::= ReturnStatement
                          | ExpressionStatement
                          | IfStatement
                          | WhileStatement
                          | Scope ;

ReturnStatement         ::= "ret" Expression? ;

ExpressionStatement     ::= Expression ;

IfStatement             ::= "if" "(" Expression ")" Scope ("else" Scope)? ;

WhileStatement          ::= "while" "(" Expression ")" Scope ;

VariableDeclaration     ::= Identificator "<-" Expression ;

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
                          | "(" Expression ")";

Literal                 ::= IntLiteral | FloatLiteral | StrLiteral ;
```
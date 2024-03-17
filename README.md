# OurScheme-Interpreter
A Lisp liked language interpreter.  

設計一個類Lisp語言直譯器，輸入為一系列S-expression，直到遇到跳脫字元停止，輸出為指令所對應的計算結果。  
OurScheme-Interpreter會先做指令讀取並切分成token，建成parsing tree的同時檢測是否有syntax error，確認該指令為合法S-expression，再以recursive做結果計算，最後印出運算結果。



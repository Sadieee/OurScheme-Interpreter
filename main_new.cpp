// 編譯:cl /EHsc main_new.cpp
# include <iostream>
# include <string>
# include <vector>
# include <cstdlib>
# include <cstring>
# include <cmath>
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <iomanip>

using namespace std;

typedef enum {
  INT,
  STR,
  DOT,
  FLOAT,     // 有正負號也算
  NIL,       // nil #f (大小寫要全對)
  T,         // t #t   (大小寫要全對)
  SYM,       // symbol (不含( ) ' " white-spaces)
  QUOTE,     // '
  LP,        // (
  RP,        // )
  NON        // 沒TOKEN
} Type 
;
typedef enum {
  INTER,
  LEAF
} NodeType
;
struct TokenNode {
  int line, column; // 此token的第一個char位置
  Type tokenType;   // 若有值 記錄其型別
  NodeType nodeType;// 若為internal node或leaf
  string token;
  float value;
  TokenNode* left;
  TokenNode* right;
  // 從此inter node開始是list/quote 
  bool isList;    
};
typedef TokenNode* Tree;
struct ResultNode {
  int type;  // 0 str 1 int 2 float
  Type tokenType;
  string str;
  float value;
};
struct UserSymNode {
  // user自訂義symbol
  string sym;
  Tree binding;
};
class Scanner {
// 此class要用來做lexical analyzer/get token
private:
  int mline, mcolumn, mtempStatus;    // 目前char的行數/列數 /
  int mpeekLine, mpeekColumn;         // 存peek char的行數/列數
  bool mnextExist, mEnd;              // 下一個是否被讀進來了 / 輸入結束
  bool mnewLineEncounter, mfirstToken;// 遇到換行 / 是否為此sexpr的第一個token(存line用)
  char mnextTempChar ;                // 暫存因為檢查，被讀進來的下一個char
  // token
  bool mtokenExist;                   // token已經被讀入(因為peek)
  Tree mnextTempToken;                // 暫存被讀進來的下一個token
  // 停止: mEnd = true ， return'\0'
  char GetChar() {
    // 用stdin讀檔方式
    // get next char，回傳一個字元 
    // keep track of 所get到的char的line numberand column number
    char nextChar ;
    if ( mnextExist ) {
      mline = mpeekLine;
      mcolumn = mpeekColumn;
      nextChar = mnextTempChar;
      mnextExist = false;
    } // if
    else {
      if ( scanf( "%c", &nextChar ) == EOF ) {
        mEnd = true;
        return '\0';
      } // if
      // 註解、error不用算
      if ( mnewLineEncounter ) {
        mline++;
        mcolumn = 1;
        mnewLineEncounter = false;
      } // if
      else 
        mcolumn++;
    } // else

    if ( nextChar == '\n' )
      mnewLineEncounter = true;
    return nextChar ;
  } // GetChar()

  char PeekChar() {
    if ( !mnextExist ) {
      // 先幫原本的char存位置
      mpeekLine = mline, mpeekColumn = mcolumn;
      mnextTempChar = GetChar();
      // 把peek和原本char的位置換回來
      int temp = mpeekLine;
      mpeekLine = mline, mline = temp;
      temp = mpeekColumn;
      mpeekColumn = mcolumn, mcolumn = temp;
      mnextExist = true;
    } // if

    return mnextTempChar;
  } // PeekChar()

  void TokenInit( Tree & newNode ) {
    newNode = new TokenNode;
    newNode->nodeType = LEAF;
    newNode->token = "";
    newNode->tokenType = NON;
    newNode->left = NULL;
    newNode->right = NULL;
    newNode->value = 0;
    newNode->isList = false;
  } // TokenInit()

  bool IsSeparator( char check ) {
    if ( check == '\'' || check == '(' || check == ')' || check == '\"' )
      return true;
    return false;
  } // IsSeparator() 

  bool IsWhiteSpace( char check ) {
    if ( check == ';' || check == ' ' || check == '\t' || check == '\n' )
      return true;
    return false;
  } // IsWhiteSpace()  

  char GetNoWhiteSpaceChar() {
    // get the next non - white - space char
    // 跳過comment(一遇到"//"，後面到\n前都視為垃圾，";"也一樣)
    // 結果為next token之始
    bool isComment = false, hasNewLine = false; // 只要有/n line要-1
    char nextChar = GetChar();
    while ( nextChar != EOF && nextChar != '\0' && 
            ( isComment || IsWhiteSpace( nextChar ) ) ) {
      if ( isComment && nextChar == '\n' )
        isComment = false ;
      else if ( nextChar == ';' )
        isComment = true ;
      if ( nextChar == '\n' )
        hasNewLine = true;
      nextChar = GetChar();
    } // while
    // 第一個token前面才需要減一個換行
    if ( hasNewLine && mfirstToken )
      mline -= 1;
    if ( nextChar == EOF || nextChar == '\0' )
      mEnd = true;
    mfirstToken = false;
    return nextChar ;
  } // GetNoWhiteSpaceChar()

  bool GetString( Tree & newNode ) {
    // true 成功 false 失敗
    char nextChar, peek;
    for ( peek = PeekChar() ; peek != EOF && peek != '\"' && peek != '\n' 
          && peek != '\0' ; ) {
      nextChar = GetChar();
      peek = PeekChar();
      // 跳脫字元
      if ( nextChar == '\\' && ( peek == '"' || peek == '\\' 
                                 || peek == 'n' || peek == 't' ) ) {
        // 把\後面那個讀進來
        nextChar = GetChar();
        if ( nextChar == 'n' )
          peek = '\n';
        else if ( nextChar == 't' )
          peek = '\t';
        newNode->token.push_back( peek ); 
        peek = PeekChar();
      } // if
      // 一般字元
      else 
        newNode->token.push_back( nextChar );
    } // for
    // 放原本應該要放的正確位置
    if ( peek == '\n' || peek == EOF || peek == '\0' ) {
      newNode->line = mline, newNode->column = mcolumn + 1;
      return false;
    } // if
    // 把"讀進來
    else 
      newNode->token.push_back( GetChar() );

    return true;
  } // GetString()

  void TypeAssign( Tree & newToken, int & status ) {
    // 判斷token type的旗幟
    string token = newToken->token;
    bool allNum = true, hasDot = false;
    if ( newToken->tokenType == STR )
      ;
    else if ( token == "(" )
      newToken->tokenType = LP;
    else if ( token == ")" )
      newToken->tokenType = RP;
    else if ( token == "\'" ) {
      newToken->token = "quote";
      newToken->tokenType = QUOTE;
    } // else if
    // DOT .
    else if ( token == "." )
      newToken->tokenType = DOT;
    // T
    else if ( token == "t" || token == "#t" ) {
      newToken->token = "#t";
      newToken->tokenType = T;
    } // else if
    // NIL
    else if ( token == "nil" || token == "#f" ) {
      newToken->tokenType = NIL;
      newToken->token = "nil";
    } // else if
    // INT FLOAT SYM
    else {
      bool isINT = true, isSYM = true, hasInterger = false; // symbol能有數字 / -.不是數字
      float value = 0;
      int dotNum = 0, digitNum = 10; // 大於0等於1 ->小數;大於1 ->error / 記錄下一個位數1、10、100、0.1...
      for ( int index = 0 ; index < token.size() ; index++ ) {
        if ( ( token[index] >= '0' && token[index] <= '9' ) ) {
          hasInterger = true;
          // 小數
          if ( dotNum == 1 ) {
            // 保留小數點的除法要先轉成double
            double tempNew = ( token[index] - '0' ), tempDigit = digitNum;
            value = value + tempNew / tempDigit;
            digitNum *= 10;
          } // if
          // 數字
          else 
            value = value * 10 + ( token[index] - '0' );
        } // if
        else if ( token[index] == '.' ) 
          dotNum += 1;
        else if ( IsSeparator( token[index] ) )
          isSYM = false, isINT = false;
        // 正負號
        else if ( ! ( index == 0 && ( token[index] == '-' || token[index] == '+' ) ) )
          isINT = false;
      } // for
      // int
      if ( isINT  && hasInterger && ( dotNum == 0 || dotNum == 1 ) ) {
        if ( token[0] == '-' )
          value = 0 - value;
        newToken->value = value;
        if ( dotNum == 0 ) 
          newToken->tokenType = INT;
        else 
          newToken->tokenType = FLOAT;
      } // if
      else if ( isSYM )
        newToken->tokenType = SYM;
      // error
      else 
        status = 2;
    } // else
  } // TypeAssign()

public:
  Scanner() {
    mline = 1, mcolumn = 0, mpeekLine = 1, mpeekColumn = 1, mtempStatus = 0;  // 目前char的行數/列數
    mnextExist = false, mtokenExist = false, mEnd = false, mnewLineEncounter = false;
    mnextTempToken = NULL, mfirstToken = true;
  } // Scanner()
  // status: 0 正常 ; 其他代表錯誤代碼
  Tree GetToken( int & status ) {
    status = 0;
    // 已經讀進來就直接回傳
    if ( mtokenExist ) {
      mtokenExist = false;
      status = mtempStatus;
      return mnextTempToken;
    } // if
    // 初始化新token
    Tree newToken = new TokenNode;
    // 只要有塞token的都是leaf
    TokenInit( newToken );
    // 取第一個非空白char
    char nextChar = GetNoWhiteSpaceChar();
    // 存此token第一個char的位置
    newToken->column = mcolumn, newToken->line = mline;
    // 沒input
    if ( mEnd || nextChar == EOF || nextChar == '\0' ) {
      mEnd = true;
      status = 4;
      return NULL;
    } // if
    // 先把第一個char丟進去
    newToken->token.push_back( nextChar ); 
    // string 停在此token最後一個char
    if ( nextChar == '\"' ) {
      newToken->tokenType = STR;
      if ( GetString( newToken ) )
        status = 0;
      else
        status = 3;
    } // if()
    // seperator 停在此token最後一個char
    else if ( !IsSeparator( nextChar ) ) 
      // 剩下讀進來
      for ( char peek = PeekChar() ; !mEnd && peek != EOF && 
            !IsSeparator( peek ) && 
            !IsWhiteSpace( peek ) ; peek = PeekChar() ) {
        nextChar = GetChar();
        newToken->token.push_back( nextChar );
      } // for
    // 判斷型別
    TypeAssign( newToken, status );
    return newToken ;
  } // GetToken()

  Tree Peek() {
    // peek不需要管status 紀錄即可
    if ( !mtokenExist ) {
      mnextTempToken = GetToken( mtempStatus );
      mtokenExist = true;
    } // if

    return mnextTempToken;
  } // Peek()

  void SkipAll() {
    // 錯誤後面全部讀掉
    char ignore = 'a';
    if ( mnextExist )
      ignore = mnextTempChar;
    while ( !mEnd && ignore != '\0' && ignore != '\n' )
      ignore = GetChar();
    mnextExist = false;
    mtokenExist = false;
  } // SkipAll()

  void Clean() { 
    // 換下一個sexpr 清空
    mfirstToken = true, mnewLineEncounter = false;
    // 因為sym會把下一個char讀進來 所以mnextExist不能重設 但行、列要
    mline = 1, mcolumn = 0, mpeekLine = 1, mpeekColumn = 1;
    // cout << "Has reset" << endl;
    // 把有被peek的token line歸1
    if ( mtokenExist )
      mnextTempToken->line = 1;
  } // Clean()
};
class ErrorHandler {
public:
  ErrorHandler() {
  } // ErrorHandler()

  void Error( int type, int line, int column, string token ) {
    if ( type == 4 )
      cout << "ERROR (no more input) : END-OF-FILE encountered" ;
    else if ( type == 3 )
      cout << "ERROR (no closing quote) : END-OF-LINE encountered at Line "
           << line << " Column " << column ;
    else if ( type == 2 )
      cout << "ERROR (unexpected token) : atom or '(' expected when token at Line " << line << " Column " 
           << column << " is >>" << token << "<<" ;
    else if ( type == 1 )
      cout << "ERROR (unexpected token) : ')' expected when token at Line "
           << line << " Column " << column << " is >>" << token << "<<";
    cout << endl;
  } // Error()

  void ToUpper( string & str ) {
    for ( int i = 0 ; i < str.length() ; i++ )
      // 因為小寫從97開始、大寫從65
      if ( str[i] > 70 )
        str[i] -= 32;
  } // ToUpper()

  bool EvalError( int type, string str, Tree sexp ) {
    bool prettyPrint = false;
    if ( type == 1 ) // format error : 有可能是因為redefining primitive之故
      cout << "ERROR (unbound symbol) : " << str ;
    else if ( type == 2 ) {
      ToUpper( str );
      cout << "ERROR (" << str << " format) : " ; 
      prettyPrint = true; // <the main S - exp>;
    } // else if
    else if ( type == 3 ) {
      cout << "ERROR (non-list) : ";
      prettyPrint = true; // (...)要pretty print 也就是印moutput
    } // else if
    else if ( type == 4 )
      cout << "ERROR (attempt to apply non-function) : " << str ;
    else if ( type == 5 ) { 
      cout << "ERROR (attempt to apply non-function) : " ;
      prettyPrint = true; // < main sxepr > ;
    } // else if
    else if ( type == 6 ) {
      ToUpper( str );
      cout << "ERROR (level of " << str << ")";
    } // else if
    else if ( type == 7 )
      cout << "ERROR (incorrect number of arguments) : " << str;
    else if ( type == 8 )
      cout << "ERROR (incorrect number of arguments) : " ; // lambda expression;
    else if ( type == 9 ) {
      cout << "ERROR (" << str << " with incorrect argument type) : " ;
      prettyPrint = true; //  ☆ is the evaluated result
    } // else if
    else if ( type == 10 )
      cout << "ERROR (no return value) : " << str; // name of this func.;
    else if ( type == 11 ) {
      cout << "ERROR (no return value) : " ; // lambda expression ; // if there is such a case ...
      prettyPrint = true;
    } // else if
    else if ( type == 12 )
      cout << "ERROR (division by zero) : " << str ;
    else if ( type == 13 ) {
      cout << "ERROR (unbound parameter) : ";
      prettyPrint = true;
    } // else if

    if ( !prettyPrint ) 
      cout << endl;
    return prettyPrint;
  } // EvalError()
};
class Parser {
  // Syntax checking first (including lexical/syntactical error detection), 
  // evaluation later      (including var.declaration)
private:
  Scanner mscanner ;
  bool misEnd, mhasError, mReduction;       // 出現quit結束 / 這個exp有error，直接捨棄sexpr 
  // 存一個s-expr
  Tree mroot;
  // 存一個output sexp
  vector<ResultNode> moutput;
  // ----------eval-----------
  vector<UserSymNode> mSym;      // 存各種user symbol，binding存該binding的"記憶體位置"!!
  vector<UserSymNode> mLocalVar; // local var.
  Tree GetToken( int & status ) {
    Tree newToken = mscanner.GetToken( status ) ;
    if ( status != 0 && status != 4 )
      Error( status, newToken->line, newToken->column, newToken->token );
    if ( status == 4 ) {
      misEnd = true;
      // newToken是null
      Error( status, 0, 0, "" );
    } // if

    return newToken ;
  } // GetToken()

  void Error( int type, int line, int column, string token ) {
    mhasError = true;
    ErrorHandler error;
    error.Error( type, line, column, token );
    mscanner.SkipAll();
    throw "error occur";
  } // Error()

  void CleanAll() {
    mroot = NULL;
    mhasError = false;
    moutput.clear();
    // 重設line、column
    mscanner.Clean();
  } // CleanAll()

  Tree InterNodeInit( Tree leftSub, Tree rightSub ) {
    Tree newNode = new TokenNode;
    newNode->nodeType = INTER;
    newNode->token = "";
    newNode->tokenType = NON;
    newNode->left = leftSub;
    newNode->right = rightSub;
    newNode->value = 0;
    newNode->isList = false;
    return newNode;
  } // InterNodeInit()

  void PrintNodeInsert( vector<ResultNode> & output, int type, string str, float value ) {
    ResultNode newNode;
    newNode.type = type;
    // value
    if ( type == 1 || type == 2 )
      newNode.value = value;
    else
      newNode.str = str;
    output.push_back( newNode );
  } // PrintNodeInsert()

  Tree Sexp( bool printLP, bool printNil, bool isAtom ) {
    int status = 0;
    // 讀入token
    Tree newNode = GetToken( status );
    // 輸入有問題 或是結束
    if ( mhasError || newNode == NULL )
      return NULL;
    // atom
    if ( newNode->tokenType == INT || newNode->tokenType == STR || newNode->tokenType == FLOAT 
         || newNode->tokenType == NIL || newNode->tokenType == T || newNode->tokenType == SYM ) {
      if ( newNode->tokenType == INT )
        PrintNodeInsert( moutput, 1, " ", newNode->value );
      else if ( newNode->tokenType == FLOAT )
        PrintNodeInsert( moutput, 2, " ", newNode->value );
      // . nil不用印
      else if ( newNode->tokenType == NIL && !printNil )
        ;
      else 
        PrintNodeInsert( moutput, 0, newNode->token, 0 );
    } // if
    else if ( newNode->tokenType == LP ) {
      isAtom = false;
      Tree nextNode = mscanner.Peek();
      // 只有()
      if ( nextNode->tokenType == RP ) {
        // .()不用印
        if ( printLP ) 
          PrintNodeInsert( moutput, 0, "nil", 0 );
        Tree abandon = GetToken( status ); // 讀掉)
        newNode->tokenType = NIL;
        newNode->token = "nil";
      } // if
      else {
        bool exitExist = false;
        // 印LP
        if ( !mhasError && printLP )
          PrintNodeInsert( moutput, 0, "(", 0 );
        // S1
        if ( !mhasError && !exitExist ) {
          newNode = InterNodeInit( Sexp( true, true, isAtom ), NULL );
          if ( mhasError )
            return newNode;
          nextNode = mscanner.Peek();
          if ( mhasError )
            return newNode;
        } // if
        // s1已經被讀進來了，就是exit，所以手動放入
        else {
          // symbol都要加換行
          PrintNodeInsert( moutput, 0, newNode->token, 0 );
          newNode = InterNodeInit( newNode, NULL );
        } // else
        // S2...Sn
        if ( !mhasError && nextNode->tokenType != DOT && nextNode->tokenType != RP ) {
          // 不只一個sexpr.
          for ( Tree now = newNode ; !mhasError && nextNode != NULL && 
                nextNode->tokenType != RP && nextNode->tokenType != DOT && 
                nextNode->tokenType != NON ; ) {
            now->right = InterNodeInit( Sexp( true, true, isAtom ), NULL );
            now = now->right;
            if ( !mhasError )
              nextNode = mscanner.Peek();
          } // for

          if ( mhasError ) 
            return newNode;
        } // if
        // .
        if ( !mhasError && nextNode->tokenType == DOT ) {
          Tree abandon = GetToken( status ); // 把DOT讀進來
          nextNode = mscanner.Peek();
          Tree now = newNode;
          while ( now->right != NULL )
            now = now->right;
          if ( nextNode->tokenType != LP && nextNode->tokenType != NIL ) {
            PrintNodeInsert( moutput, 0, ".", 0 );
            // 放右子樹
            now->right = Sexp( true, true, isAtom );
          } // if
          // 因為NIL即是右子樹 不用遞迴 .nil 
          else if ( nextNode->tokenType == NIL )
            now->right = Sexp( false, false, isAtom );
          else 
            // .()和.(不用印 
            now->right = Sexp( false, true, isAtom );
          if ( mhasError || now->right == NULL )
            return newNode;
        } // if
        // )
        nextNode = mscanner.Peek();
        if ( !mhasError && nextNode->tokenType == RP ) {
          // 前面若沒印( 則不用印)
          if ( printLP ) 
            PrintNodeInsert( moutput, 0, ")", 0 );
          Tree abandon = GetToken( status );     // 把)讀進來
        } // if
        else if ( !mhasError )
          Error( 1, nextNode->line, nextNode->column, nextNode->token );
      } // else
    } // else if
    else if ( !mhasError && newNode->tokenType == QUOTE ) {
      // '()是 nil
      isAtom = false;
      PrintNodeInsert( moutput, 0, "(", 0 );
      PrintNodeInsert( moutput, 0, "quote", 0 );
      newNode = InterNodeInit( newNode, InterNodeInit( Sexp( true, true, isAtom ), NULL ) );
      PrintNodeInsert( moutput, 0, ")", 0 );
    } // else if
    else if ( !mhasError )
      Error( 2, newNode->line, newNode->column, newNode->token );
    return newNode;
  } // Sexp()

  void GetSexpr() {
    // 停止條件:遇到error、end of file、讀到完整的s - exp
    cout << "\n> ";
    mroot = Sexp( true, true, true );
  } // GetSexpr()
  // -----------------------Print------------------------------
  void InorderRecur( Tree now, bool printSpace, int space, bool printRp, bool isLeftSub ) {
    // isLeftsub->可以print ( ， 
    int index;
    if ( now == NULL )
      return;
    // LEAF
    if ( now->nodeType == LEAF ) {
      // print space
      if ( printSpace )
        for ( int j = 0 ; j < space ; j++ )
          cout << " " ;
      // print token
      if ( now->tokenType == INT )
        cout << fixed << setprecision( 0 ) << now->value;
      else if ( now->tokenType == FLOAT )
        cout << fixed << setprecision( 3 ) << now->value;
      else
        cout << now->token;
      cout << endl;
      // print rp
      if ( printRp ) {
        space -= 2;
        for ( int j = 0 ; j < space ; j++ )
          cout << " " ;
        cout << ")\n";
      } // if

      return;
    } // if
    // internal node print lp
    else if ( isLeftSub ) {
      if ( printSpace )
        for ( int j = 0 ; j < space ; j++ )
          cout << " " ;
      cout << "( ";
      space = space + 2;
    } // else if

    if ( now->left != NULL ) {
      bool nextSpace = ( isLeftSub ) ? false : true; 
      // cout << "space end: " << space << endl;
      InorderRecur( now->left, nextSpace, space, false, true );
      
    } // if
    else
      cout << "error occur!" << endl;

    if ( now->right != NULL && now->right->tokenType != NIL ) {
      // 下一個是leaf or null，且此node一定是inter
      if ( now->right->nodeType == LEAF ) {
        // if ( printSpace )
        for ( int j = 0 ; j < space ; j++ )
          cout << " ";
        cout << "." << endl;
        printRp = true;
      } // if

      InorderRecur( now->right, true, space, printRp, false );
    } // if
    else if ( now->nodeType == INTER ) {
      // print rp
      space = space - 2;
      for ( int j = 0 ; j < space ; j++ )
        cout << " " ;
      cout << ")\n";
    } // else if

  } // InorderRecur()
  // error的pretty print不用印procedure
  void PrintRecur( Tree sexpr ) {
    bool isLeftSub = true, printRp = true;
    if ( sexpr != NULL && sexpr->nodeType == LEAF )
      isLeftSub = false, printRp = false;
    InorderRecur( sexpr, true, 0, printRp, isLeftSub );
  } // PrintRecur()
  // print vector
  void PrettyPrint() { 
    Print( moutput );
  } // PrettyPrint()

  void Print( vector<ResultNode> output ) {
    int space = 0;
    bool printSpace = true;
    for ( int i = 0 ; i < output.size() ; i++ ) {
      if ( output[i].str == ")" ) 
        space -= 2;
      // 印空格
      for ( int j = 0 ; printSpace && j < space ; j++ )
        cout << " " ;
      // 印字串
      if ( output[i].type == 1 )
        cout << fixed << setprecision( 0 ) << output[i].value ;
      else if ( output[i].type == 2 )
        cout << fixed << setprecision( 3 ) << output[i].value;
      else
        cout << output[i].str;
      if ( output[i].str == "(" ) {
        cout << " ";
        space += 2;
        printSpace = false;
      } // if
      else {
        // 印換行
        printSpace = true;
        cout << endl;
      } // else
    } // for
  } // Print()
  // --------------------------evaluate-------------------------------------
  void BackUp( vector<UserSymNode> & table, vector<UserSymNode> & temp ) {
    temp.clear();
    for ( int i = 0 ; i < table.size() ; i++ ) {
      UserSymNode newNode;
      newNode.sym = table[i].sym;
      newNode.binding = CopyPointer( table[i].binding );
      temp.push_back( newNode );
    } // for

    table.clear();
  } // BackUp()

  void ReDuction( vector<UserSymNode> & table, vector<UserSymNode> & temp ) {
    table.clear();
    for ( int i = 0 ; i < temp.size() ; i++ ) {
      UserSymNode newNode;
      newNode.sym = temp[i].sym;
      newNode.binding = CopyPointer( temp[i].binding );
      table.push_back( newNode );
    } // for
  } // ReDuction()

  Tree EvalError( int type, string sym, Tree sexpr ) { 
    mhasError = true;
    ErrorHandler error;
    // true->pretty print
    if ( error.EvalError( type, sym, sexpr ) ) 
      PrintRecur( sexpr );
    throw "error occur";
    return NULL;
  } // EvalError()
  // 0回傳0、
  bool Min( bool op1, bool op2 ) {
    if ( op1 == false || op2 == false )
      return false;
    else return true;
  } // Min()

  bool IsArith( string symbol ) {
    if ( symbol == "+" || symbol == "*" || symbol == "/" || symbol == "-" )
      return true;
    return false;
  } // IsArith()

  bool IsAtom( Tree sexp ) {
    if ( sexp->isList )
      return false;
    if ( sexp != NULL && sexp->nodeType == LEAF &&
         ( sexp->tokenType == INT || sexp->tokenType == FLOAT ||
           sexp->tokenType == STR || sexp->tokenType == T ||
           sexp->tokenType == NIL ) )
      return true;
    else
      return false;
  } // IsAtom()

  bool IsSystemSym( string symbol ) {
    string sym = symbol;
    if ( IsProcedureSym( sym ) )
      sym = ProcedureToSystem( sym );
    if ( sym == "define" || sym == "list" || sym == "lambda" ||
         sym == "cond" || sym == "cons" ||  sym == "atom?" || 
         sym == "cdr" || sym == "car" || sym == "quote" || sym == "let" ||
         sym == "exit" || sym == "clean-environment" || sym == "pair?" || sym == "null?" || 
         sym == "integer?" || sym == "real?" || sym == "number?" || sym == "boolean?" ||
         sym == "string?" || sym == "symbol?" || sym == "not" || sym == "+" || sym == "-" || 
         sym == "*" || sym == "/" || sym == ">" || sym == "<" || sym == "=" || sym == ">=" ||
         sym == "<=" || sym == "string-append" || sym == "string>?" || sym == "string<?" || 
         sym == "string=?" || sym == "string>=?" || sym == "string<=?" || sym == "eqv?" || 
         sym == "equal?" || sym == "if" || sym == "and" || sym == "or" ||
         sym == "list?" || sym == "begin" ) //   ||  sym == "set!" 
      return true;
    return false;
  } // IsSystemSym()

  bool IsUserSym( string symbol, int & index ) {
    string sym = symbol;
    if ( IsProcedureSym( sym ) )
      sym = ProcedureToSystem( sym );
    for ( int i = 0 ; i < mSym.size() ; i++ )
      if ( sym == mSym[i].sym ) {
        index = i;
        return true;
      } // if

    return false;
  } // IsUserSym()

  bool IsLocalSym( string symbol, int & index ) {
    string sym = symbol;
    if ( IsProcedureSym( sym ) )
      sym = ProcedureToSystem( sym );
    for ( int i = 0 ; i < mLocalVar.size() ; i++ )
      if ( sym == mLocalVar[i].sym ) {
        index = i;
        return true;
      } // if

    return false;
  } // IsLocalSym()
  // 確認此symbol是否為user/system symbol
  bool IsBounded( string symbol, int & index ) {
    if ( IsSystemSym( symbol ) || IsLocalSym( symbol, index ) || IsUserSym( symbol, index ) )
      return true;
    return false;
  } // IsBounded() 

  bool IsExistSym( vector<UserSymNode> table, string symbol, int & index ) {
    string sym = symbol;
    if ( IsProcedureSym( sym ) )
      sym = ProcedureToSystem( sym );
    for ( int i = 0 ; i < table.size() ; i++ )
      if ( sym == table[i].sym ) {
        index = i;
        return true;
      } // if

    return false;
  } // IsExistSym()

  bool IsSym( Tree now ) {
    // 只接受sym || null
    if ( now == NULL )
      return true;
    if ( now->nodeType == LEAF && now->tokenType != SYM )
      return false;
    else if ( !IsSym( now->left ) || !IsSym( now->right ) )
      return false;
    return true;
  } // IsSym()
  // 確認是否有未binding sym，且印出其值，自動跳過quote
  bool IsAllBound( Tree now ) {
    if ( now == NULL )
      return true;
    else if ( now->nodeType == LEAF ) {
      int index;
      if ( now->tokenType == SYM && !IsBounded( now->token, index ) && !now->isList ) {
        EvalError( 1, now->token, NULL );
        return false;
      } // if
      else return true;
    } // else if
    else if ( IsQuote( now, 2 ) || ( IsAllBound( now->left ) && IsAllBound( now->right ) ) )
      return true;
    return false;
  } // IsAllBound()
  // 產生一個true node
  Tree TrueNodeInit() {
    Tree newNode = new TokenNode;
    newNode->token = "#t";
    newNode->tokenType = T;
    newNode->nodeType = LEAF;
    newNode->left = NULL;
    newNode->right = NULL;
    newNode->isList = false;
    return newNode;
  } // TrueNodeInit()

  Tree FalseNodeInit() {
    Tree newNode = new TokenNode;
    newNode->token = "nil";
    newNode->tokenType = NIL;
    newNode->nodeType = LEAF;
    newNode->left = NULL;
    newNode->right = NULL;
    newNode->isList = false;
    return newNode;
  } // FalseNodeInit()

  Tree IntNodeInit( float num ) {
    Tree newNode;
    TokenInit( newNode );
    newNode->tokenType = INT;
    newNode->right = NULL;
    newNode->left = NULL;
    newNode->value = num;
    newNode->isList = false;
    return newNode;
  } // IntNodeInit()
  // 確認是否為list(car)，不能有.，
  bool IsPureList( Tree sexpr ) { 
    if ( sexpr == NULL )
      return true;
    // .NIL沒關係
    for ( Tree temp = sexpr ; temp->right != NULL ; temp = temp->right ) {
      if ( temp->right != NULL && temp->right->nodeType == LEAF && temp->right->tokenType != NIL )
        return false;
      else if ( temp->right->tokenType == NIL ) {
        temp->right = NULL;
        return true;
      } // else if
    } // for

    return true;
  } // IsPureList()
  // 是quote list，回傳quote
  bool IsQuote( Tree now, int level ) {
    if ( now == NULL )
      return false;
    if ( now->left != NULL && now->left->tokenType == QUOTE ) {
      // now = Evaluate( now, level );
      return true;
    } // if
    else if ( now->isList ) return true;
    return false;
  } // IsQuote()
  // 是quote || 括號中都是atom 或 user define sym
  bool IsList( Tree now, int level ) {
    if ( now == NULL || IsQuote( now, level ) || now->isList )
      return true;
    else {
      bool isList;
      int index;
      // 處理左
      if ( now->left != NULL ) {
        if ( now->left->nodeType == INTER )
          isList = IsList( now->left, level + 1 );
        else if ( IsAtom( now->left ) || IsUserSym( now->left->token, index ) || 
                  IsLocalSym( now->left->token, index ) || now->left->token == "list" )
          isList = true;
        else isList = false;
      } // if
      // 遞迴右
      isList = Min( isList, IsList( now->right, level ) );
      return isList;
    } // else
  } // IsList()
  // 給兩個sub 看是int or float
  Tree NumberType( Tree newNode, Tree leftsub, Tree rightsub ) {
    if ( ( leftsub != NULL && leftsub->tokenType == FLOAT ) ||
         ( rightsub != NULL && rightsub->tokenType == FLOAT ) )
      newNode->tokenType = FLOAT;
    else
      newNode->tokenType = INT;
    return newNode;
  } // NumberType()

  void TokenInit( Tree & newNode ) {
    newNode = new TokenNode;
    newNode->nodeType = LEAF;
    newNode->token = "";
    newNode->tokenType = NON;
    newNode->left = NULL;
    newNode->right = NULL;
    newNode->value = 0;
    newNode->isList = false;
  } // TokenInit()
  // 確認sexpr.參數個數是否正確
  bool CheckArg( int argu, Tree sexp ) {
    int arguNum = ArguNum( sexp );
    if ( arguNum == argu )
      return true;
    return false;
  } // CheckArg()
  // 算有幾個argu(就是走訪右pointer，每多一條left pointer則argu++)
  int ArguNum( Tree sexp ) {
    if ( sexp == NULL )
      return 0;
    else if ( sexp->nodeType == LEAF || sexp->isList )
      return 1;
    else if ( sexp->left != NULL )
      return ( 1 + ArguNum( sexp->right ) );
    else
      return ArguNum( sexp->right );
  } // ArguNum()
  // symbol不用同一個，值一樣就好
  bool IsSame( Tree op1, Tree op2 ) {
    if ( op1 == NULL && op2 == NULL ) return true;
    else if ( op1 == NULL || op2 == NULL ) return false;

    return false;
  } // IsSame() 
  // 比較兩子樹 要每個元素一樣 
  bool IsSameTree( Tree op1, Tree op2 ) {
    int index;
    if ( op1 == NULL && op2 == NULL ) 
      return true;
    if ( op1 == NULL || op2 == NULL ) return false;
    else if ( op1->nodeType == INTER && op2->nodeType == INTER )
      return Min( IsSameTree( op1->left, op2->left ), IsSameTree( op1->right, op2->right ) );
    else if ( op1->nodeType == LEAF && op2->nodeType == LEAF ) {
      if ( op1->tokenType == op2->tokenType && 
           ( ( op1->token == op2->token ) ||
             ( op1->tokenType == INT && op1->value == op2->value ) ||
             ( op1->tokenType == FLOAT && op1->value == op2->value ) ) ) return true;
    } // else if

    return false;
  } // IsSameTree()
  // 全是exit || nil || . || quote )
  bool IsWhiteSpaceStr( Tree str ) {
    if (  str == NULL || ( str->nodeType == LEAF && ( str->tokenType == NIL || str->tokenType == QUOTE || 
                                                      str->tokenType == DOT || str->tokenType == NON ) ) )
      return true;
    return false;
  } // IsWhiteSpaceStr()
  // 是否為#<procedure形式
  bool IsProcedureSym( string str ) {
    size_t found = str.find( "#<procedure" );
    if ( found != string::npos ) return true;
    return false;
  } // IsProcedureSym()
  // car ->  #<procedure car>
  string SystemToProcedure( string token ) {
    return "#<procedure " + token + ">";
  } // SystemToProcedure()
  // #<procedure car> -> car
  string ProcedureToSystem( string str ) {
    string newStr = "\0";
    for ( int i = 12 ; i < str.length() - 1 ; i++ )
      newStr.push_back( str[i] );
    return newStr;
  } // ProcedureToSystem()
  // 去掉str的最後一個
  string PopBack( string str ) {
    string newstr = "";
    for ( int i = 0 ; i < str.length() - 1 ; i++ )
      newstr.push_back( str[i] );
    return newstr;
  } // PopBack()
  // 複製一個長得一模一樣的pointer
  Tree CopyPointer( Tree ptr ) {
    if ( ptr == NULL )
      return NULL;
    else if ( ptr->nodeType == LEAF ) {
      Tree newNode;
      TokenInit( newNode );
      newNode->tokenType = ptr->tokenType;
      newNode->value = ptr->value;
      newNode->token = ptr->token;
      newNode->isList = ptr->isList;
      newNode->token = ptr->token;
      return newNode;
    } // else if
    else {
      Tree newNode = InterNodeInit( CopyPointer( ptr->left ), CopyPointer( ptr->right ) );
      newNode->isList = ptr->isList;
      return newNode;
    } // else
  } // CopyPointer()
  // 定義變數
  void DefineParameter( vector<UserSymNode> & table, Tree parameter, string sym ) {
    // 定義參數
    for ( int index ; parameter != NULL && parameter->tokenType != NIL ; parameter = parameter->right ) {
      Tree var = parameter->left;
      if ( var != NULL && var->tokenType == SYM && !IsBounded( var->token, index ) ) 
        Define( table, true, var->token, NULL );
      else if ( !IsBounded( var->token, index ) ) EvalError( 2, sym, parameter );
    } // for
  } // DefineParameter()
  // 比較兩樹是否argu一樣(nil = null)
  bool CheckSameArgu( Tree tree1, Tree tree2 ) {
    if ( ArguNum( tree1 ) == ArguNum( tree2 ) ) return true;
    else if ( ( tree1 == NULL || tree1->tokenType == NIL ) && 
              ( tree2 == NULL || tree2->tokenType == NIL ) ) return true;
    return false;
  } // CheckSameArgu()
  // 取得binding/ function->copy, 其他直接回傳
  Tree GetBinding( string & token ) {
    int index;
    Tree binding = NULL ;
    if ( IsUserSym( token, index ) )
      binding = mSym[index].binding;
    if ( IsLocalSym( token, index ) )
      binding = mLocalVar[index].binding;
    // list/function
    if ( binding != NULL && binding->token == "#<procedure lambda>" ) {
      token = "lambdaTemp";
      binding = GetBinding( token );
      token = "lambda";
    } // if

    while ( IsLocalSym( binding->token, index ) || IsUserSym( binding->token, index ) ) {
      token = binding->token;
      if ( IsProcedureSym( token ) ) token = ProcedureToSystem( token );
      binding = GetBinding( token );
    } // while

    if ( binding != NULL && binding->nodeType == INTER ) {
      if ( binding->isList ) return binding;
      else return CopyPointer( binding );
    } // if

    return binding;
  } // GetBinding()
  // 遞迴找答案值 (參數為此symbol與指著RP的指標)
  Tree EvalRecur( Tree inSexp, int level ) {
    string symbol = inSexp->left->token;
    // procedure先換過來
    if ( IsProcedureSym( symbol ) ) {
      size_t found = symbol.find( "#<procedure" ) ;
      if ( found != string::npos ) 
        symbol = ProcedureToSystem( symbol );
    } // if
    // 目標:用symbol把now的左、右合成一個值存在token node並回傳
    Tree now = inSexp->right, result;
    // 計算
    if ( symbol == "define" ) {
      if ( now != NULL && now->left != NULL && !IsSystemSym( now->left->token ) ) {
        // 一組define
        if ( now->right != NULL && now->left->tokenType == SYM ) {
          // (define hello "123" . nil)
          if ( CheckArg( 2, now ) || ( CheckArg( 3, now ) && now->right->right != NULL &&
                                       now->right->right->nodeType == LEAF ) ) {
            Tree sym = now->left, binding = now->right->left;
            if ( binding == NULL )
              cout << "binding format error" << endl; // binding沒東西
            if ( mhasError ) return NULL;
            result = Define( mSym, true, sym->token, binding );
          } // if 
          else EvalError( 2, symbol, inSexp );
        } // if
        // 多組
        else if ( now->left->nodeType == INTER ) {
          Tree sym = now->left, bindingFunction = InterNodeInit( now->left->right, now->right->left );
          // 定義function
          if ( sym->left->tokenType == SYM )
            result = Define( mSym, false, sym->left->token, bindingFunction );
          else cout << "non-symbol of function name" << endl;
          // 定義參數
          // DefineParameter( mSym, sym->right, "define" );
        } // else if
        else
          EvalError( 2, symbol, inSexp );
      } // if
      else 
        // define到system symbol || 第一個op不是sym
        EvalError( 2, symbol, inSexp );
    } // if
    else if ( symbol == "car" || symbol == "cdr" ) {
      // now是整理好的list
      if ( now == NULL ) return EvalError( 9, symbol, now );
      if ( now->nodeType == LEAF ) 
        result = now ;
      else if ( symbol == "car" )
        result = now->left;
      else if ( now->right == NULL )
        result = FalseNodeInit(); // 回傳空
      else result = now->right;
      if ( result->nodeType == INTER ) result->isList = true;
      else result->isList = false;
    } // else if
    else if ( symbol == "cons" ) 
      result = Cons( now, level );
    else if ( symbol == "list" ) {
      // 不管給幾個arg 全部串起來
      if ( IsPureList( now ) ) 
        result = List( now, level ) ;
      else
        return EvalError( 3, symbol, inSexp );
    } // else if
    else if ( symbol == "exit" ) {
      bool get = Exit( now, level );
      if ( get ) return TrueNodeInit();
      else return FalseNodeInit();
    } // else if
    else if ( symbol == "pair?" )
      // true->#t，false->nil
      return Pair( now->left );
    else if ( symbol == "null?" )
      return Null( now->left );
    else if ( symbol == "integer?" )
      return Interger( now->left );
    else if ( symbol == "real?" )
      return Real( now->left );
    else if ( symbol == "number?" )
      return Number( now->left );
    else if ( symbol == "boolean?" )
      return Bool( now->left );
    else if ( symbol == "string?" )
      return String( now->left );
    else if ( symbol == "symbol?" )
      return Symbol( now->left );
    else if ( symbol == "atom?" )
      return Atom( now->left );
    else if ( symbol == "not" )
      return Not( now->left );
    else if ( symbol == "+" || symbol == "-" || symbol == "*" || symbol == "/" )
      return Arithmetic( symbol, symbol, now, level );
    else if ( symbol == ">" || symbol == "<" || symbol == "=" || symbol == ">=" || symbol == "<=" )
      return Compare( symbol, now, level );
    else if ( symbol == "string-append" )
      return StrAppend( now, level );
    else if ( symbol == "string>?" || symbol == "string<?" || symbol == "string=?" ||
              symbol == "string>=?" || symbol == "string<=?" )
      return StrCompare( symbol, now, level );
    else if ( symbol == "eqv?" ) 
      return Eqv( now, level );
    else if ( symbol == "equal?" )
      return Equal( now, level );
    else if ( symbol == "if" )
      return If( now, level );
    else if ( symbol == "and" )
      return And( now, level );
    else if ( symbol == "or" )
      return Or( now, level );
    else if ( symbol == "cond" ) 
      return Cond( now, level );
    else if ( symbol == "begin" )
      return Begin( now, level );
    else if ( symbol == "list?" ) {
      now = now->left;
      // cdr是list || nil
      if ( IsList( now->right, level ) )
        result = TrueNodeInit() ;
      else result = FalseNodeInit();
    } // else if
    else if ( symbol == "let" ) 
      return Let( now, level );
    else if ( symbol == "lambda" ) 
      return Lambda( now, level + 1 );

    return result;
  } // EvalRecur()
  // 拆解level 1
  Tree Evaluate( Tree inSexp, int level ) {
    Tree result = NULL;
    if ( inSexp == NULL || ( inSexp->nodeType == LEAF && inSexp->tokenType == NON ) )
      result = NULL;
    // 只有atom 直接回傳(3、-5、"asc"等這種非symbol) 或 此inSexp是list || quote
    else if ( IsAtom( inSexp ) || inSexp->isList )
      result = inSexp;
    // 只有symbol
    else if ( inSexp->nodeType == LEAF && inSexp->tokenType == SYM ) {
      int symIndex = 0;
      // system symbol || not exist
      if ( !IsLocalSym( inSexp->token, symIndex ) && !IsUserSym( inSexp->token, symIndex ) ) {
        // 換成procedure的形式
        if ( IsSystemSym( inSexp->token ) ) {
          if ( !IsProcedureSym( inSexp->token ) )
            inSexp->token = SystemToProcedure( inSexp->token );
          result = inSexp;
        } // if
        else return EvalError( 1, inSexp->token, NULL );
      } // if
      // 是s-expr或裡面的function , 回傳他Binding的東西
      else {
        Tree binding = NULL;
        if ( IsUserSym( inSexp->token, symIndex ) )
          binding = mSym[symIndex].binding;
        if ( IsLocalSym( inSexp->token, symIndex ) )
          binding = mLocalVar[symIndex].binding;
        // binding 是 function->回傳#<procedure symbolname>
        if ( binding != NULL && binding->nodeType != LEAF && !binding->isList &&
             !IsProcedureSym( inSexp->token ) ) {
          inSexp->token = SystemToProcedure( inSexp->token );
          result = inSexp;
        } // if
        // list/atom
        else result = binding;
      } // else 
    } // else if
    // main sexpr.(不能是nil、atom，因為nil是atom)
    else {
      Tree s1 = inSexp->left;
      string op = s1->token ; // 動op不會動到s1
      // procedure先換過來
      if ( op == "#<procedure lambda>" )
        op = "lambdaTemp";
      else if ( s1->tokenType == SYM && IsProcedureSym( op ) ) {
        size_t found = op.find( "#<procedure" ) ;
        if ( found != string::npos ) 
          op = ProcedureToSystem( op );
      } // if
      // 算S1
      if ( op == "quote" ) {
        if ( CheckArg( 1, inSexp->right ) ) {
          // quote -> 直接印 不用計算(代表一個list)] 傳回去掉quote的list / 'good 這種直接印token
          if ( inSexp->right != NULL ) {
            result = inSexp->right->left; // ->right->left;
            result->isList = true;
          } // if

          else cout << "Empty quote" << endl;
        } // if
        else return EvalError( 7, op, NULL );
      } // if          
      // 是list
      else if ( s1->isList ) return s1;
      // 有點.
      else if ( !IsPureList( inSexp ) ) return EvalError( 3, op, inSexp );
      // 此sexpr的第一個是symbol
      else if ( s1 != NULL && s1->tokenType == SYM ) {
        int index;
        // internal funtion
        if ( IsSystemSym( op ) ) { // && ! ( op == "lambda" && IsUserSym( op, index ) ) ) {
          if ( level != 1 && ( op == "clean-environment" ||
                               op == "exit" || op == "define" ) ) 
            return EvalError( 6, op, NULL );
          // 清空環境
          else if ( op == "clean-environment" ) { 
            mReduction = false;
            mSym.clear();
            TokenInit( result );
            result->token = "environment cleaned";
            result->tokenType = STR;
          } // else if
          else if ( op == "exit" ) {
          // 可接受exit和nil
            Tree isExit = EvalRecur( inSexp, level );
            if ( isExit->tokenType == T )
              misEnd = true;
            else return EvalError( 7, op, inSexp );
            return isExit;
          } // else if
          else if ( op == "define" || op == "list" ) {
            result = EvalRecur( inSexp, level ); // 算值並結束
            if ( op == "define" ) mReduction = false;
          } // if
          else if ( op == "cond" ) {
            // >= 1
            if ( !CheckArg( 0, inSexp->right ) ) {
              // 檢查各條件參數 確認一組中是否兩個arg
              for ( Tree temp = inSexp->right ; temp != NULL ; temp = temp->right )
                if ( CheckArg( 1, temp->left ) || CheckArg( 0, temp->left ) ) 
                  return EvalError( 2, op, inSexp );
              result = EvalRecur( inSexp, level ); // 算值並結束
            } // if
            else return EvalError( 2, op, inSexp );
          } // else if
          else if ( op == "begin" ) {
            // >= 1
            // 求每個argu的值 但只印最後一個(?
            if ( !CheckArg( 0, inSexp->right ) ) 
              result = EvalRecur( inSexp, level ); // 算值並結束
            else return EvalError( 7, op, NULL );
          } // else if
          else if ( op == "string>?" || op == "string<?" || op == "string=?" || 
                    op == "string>=?" || op == "string<=?" || op == "string-append" ) {
            // arg全為str
            if ( !CheckArg( 1, inSexp->right ) && !CheckArg( 0, inSexp->right ) ) 
              result = EvalRecur( inSexp, level ); // 算值並結束
            else return EvalError( 7, op, NULL );
          } // else if
          else if ( op == "cdr" || op == "car" ) {
            // cdr 刪除第一個元素 並回傳剩下list
            // car 傳回list中第一個元素(已展開)，不能是nil，且第一個元素也不能空著
            // 一定要為list 不能是atom 但可以是sym(sym不能是atom)
            // (car 'WarAndPeace!) -> error
            if ( CheckArg( 1, inSexp->right ) ) {
              Tree list = Evaluate( inSexp->right->left, level + 1 ) ;
              // 不能為atom和'WarAndPeace!
              int index;
              if ( IsAtom( list ) || list->tokenType == SYM )
                return EvalError( 9, op, list );
              inSexp->right = list;
              result = EvalRecur( inSexp, level ); // 算值並結束
            } // if
            else return EvalError( 7, op, NULL );
          } // else if
          else if ( op == "eqv?" || op == "equal?" ) {
            // eqv 一定是兩個sym指向同一個東西 其他都不行 / equal 一樣就行
            // check 參數個數是否正確
            if ( CheckArg( 2, inSexp->right ) ) 
              result = EvalRecur( inSexp, level );
            else 
              return EvalError( 7, op, NULL );
          } // else if
          else if ( op == "if" ) {
            if ( CheckArg( 3, inSexp->right ) || CheckArg( 2, inSexp->right ) ) 
              result = EvalRecur( inSexp, level );
            else 
              return EvalError( 7, op, NULL );
          } // else if
          else if ( op == "pair?" || op == "null?" || op == "integer?" || 
                    op == "real?" || op == "number?" || op == "boolean?" ||
                    op == "string?" || op == "symbol?" || op == "not" || 
                    op == "list?" || op == "atom?" ) {
            // 檢查參數個數
            if ( CheckArg( 1, inSexp->right ) ) {
              if ( IsAllBound( inSexp->right ) )
                result = EvalRecur( inSexp, level ); // 算值並結束
              else return NULL;
            } // if
            else
              return EvalError( 7, op, NULL );
          } // else if
          // 把兩個塞在一起
          else if ( op == "cons" ) {
            // 檢查參數個數
            if ( CheckArg( 2, inSexp->right ) ) {
              if ( IsPureList( inSexp->right ) )
                // 參數型別在遞迴中檢查
                result = EvalRecur( inSexp, level ); // 算值並結束
              else return EvalError( 3, op, inSexp );
            } // if
            else return EvalError( 7, op, NULL );
          } // else if
          else if ( op == "+" || op == "-" || op == "*" || 
                    op == "/" || op == ">" || 
                    op == "<" || op == "=" || op == ">=" || 
                    op == "<=" || op == "and" || op == "or" ) {
            // 不限arg個數(>= 2)
            if ( !CheckArg( 1, inSexp->right ) && !CheckArg( 0, inSexp->right ) ) {
              result = EvalRecur( inSexp, level ); // 算值並結束
            } // if
            else return EvalError( 7, op, NULL );
          } // else if
          else if ( op == "let" || op == "lambda" ) {
            // >= 2
            if ( !CheckArg( 0, inSexp->right ) && !CheckArg( 1, inSexp->right ) && 
                 ( inSexp->right->left->nodeType == INTER || inSexp->right->left->tokenType == NIL ) )
              result = EvalRecur( inSexp, level );
            else return EvalError( 2, op, inSexp );
          } // else if
          else {
            cout << op;
            cout << "WTF" << endl;
            throw "error occur";
          } // else
        } // if
        else if ( IsLocalSym( op, index ) || IsUserSym( op, index ) ) {
          // 把binding function展開
          inSexp->left = GetBinding( op );
          if ( inSexp->left->isList ) return EvalError( 5, inSexp->left->token, inSexp->left );
          // function，inSexp->right是此function的local變數
          else if ( inSexp->left != NULL && inSexp->left->nodeType == INTER ) {  
            Tree parameter = inSexp->left->left;
            // 先define local變數
            if ( CheckSameArgu( inSexp->right, parameter ) )
              for ( Tree var = inSexp->right ; var != NULL && parameter != NULL ; 
                    var = var->right, parameter = parameter->right ) {
                Tree sym = parameter->left;
                Tree binding = var->left;
                // 解決先後取的順序問題
                if ( IsUserSym( binding->token, index ) ) 
                  binding = mSym[index].binding;
                Define( mLocalVar, true, sym->token, binding );
              } // for
          
            else return EvalError( 7, op, inSexp );
            // 求user define function
            result = Evaluate( inSexp->left->right, level + 1 );
          } // else if
          // symbol
          else if ( IsSystemSym( inSexp->left->token ) ) return Evaluate( inSexp, level + 1 );
          else return Evaluate( inSexp->left, level + 1 );
        } // else if
        else 
          return EvalError( 1, op, NULL );
      } // else if
      // 此sexpr的第一個是atom  
      else if ( s1 != NULL && IsAtom( s1 ) ) 
        return EvalError( 4, s1->token, inSexp );
      // 此sexpr的第一個是( 。。。 )(ex. ( ( 。。。 ) ...... ))
      else if ( s1->nodeType == INTER ) { 
        // 計算(。。。) result
        inSexp->left = Evaluate( s1, level + 1 );
        if ( inSexp->left->tokenType != SYM && !inSexp->left->isList ) 
          return EvalError( 5, inSexp->left->token, inSexp->left );
        result = Evaluate( inSexp, level );
      } // else if
      else {
        cout << "Unknown error occur!" << endl;
        mhasError = true;
        throw "error occur";
      } // else
    } // else

    return result;
  } // Evaluate()
  // ---------------------------------
  // 執行define binding可以是system symbol(例如a = cons)
  Tree Define( vector<UserSymNode> & table, bool eval, string sym, Tree binding ) {
    // 檢查是否已存在
    int index;
    if ( eval && !binding->isList ) {
      vector<UserSymNode> temp;
      BackUp( mLocalVar, temp );
      binding = Evaluate( binding, 2 );
      ReDuction( mLocalVar, temp );
    } // if

    if ( eval && !IsAllBound( binding ) )
      return NULL;
    // 存symbol table
    if ( IsExistSym( table, sym, index ) )
      table[index].binding = binding;
    else {
      UserSymNode newNode;
      newNode.binding = binding;
      newNode.sym = sym;
      table.push_back( newNode );
    } // else
    // 回傳的字串
    Tree returnNode;
    TokenInit( returnNode );
    returnNode->token = sym + " defined";
    returnNode->tokenType = STR;
    return returnNode;
  } // Define()
  // 把兩個sexp塞在一起，OP2是nil不用，可接受argu是atom
  Tree Cons( Tree now, int level ) {
    Tree op1 = now->left, op2 = now->right->left;
    if ( !op1->isList && !IsAtom( op1 ) ) 
      op1 = Evaluate( op1, level + 1 );
    if ( !op2->isList && !IsAtom( op2 ) )
      op2 = Evaluate( op2, level + 1 );
    if ( op2 != NULL && op2->tokenType == NIL )
      op2 = NULL;
    Tree newNode = InterNodeInit( op1, op2 );
    newNode->isList = true;
    return newNode;
  } // Cons()
  // 把binding展開 計算各sub 檢查未bound sym 'quote'不用印
  // 把arg1...argn全部串起來
  Tree List( Tree now, int level ) {
    // 每個argu可以是:quote、atom但不能是(3 4 5)這種
    // 那sym要展開?
    if ( now == NULL )
      return FalseNodeInit(); // 就算nil也可以
    // 展開
    now->isList = true;
    if ( now->nodeType == LEAF ) 
      return Evaluate( now, level );
    // INTER
    int index;
    if ( now->left == NULL ) return FalseNodeInit();
    now->left = Evaluate( now->left, level + 1 );
    now->right = List( now->right, level );
    if ( now->right != NULL && now->right->tokenType == NIL ) now->right = NULL;
    return now;
  } // List()
  // 由car、cdr組成，不能是atom(除非' || list)，但不能是nil
  Tree Pair( Tree now ) {
    // 只有sym
    now = Evaluate( now, 2 );
    if ( now == NULL || ( now != NULL && now->nodeType == LEAF ) )
      return FalseNodeInit();
    return TrueNodeInit();
  } // Pair()
 
  Tree Null( Tree now ) {
    now = Evaluate( now, 2 );
    if ( now == NULL || now->tokenType == NIL )
      return TrueNodeInit();
    return FalseNodeInit();
  } // Null()

  Tree Interger( Tree now ) {
    now = Evaluate( now, 2 );
    if ( now != NULL && now->nodeType == LEAF && now->tokenType == INT )
      return TrueNodeInit();
    return FalseNodeInit();
  } // Interger()

  Tree Real( Tree now ) {
    now = Evaluate( now, 2 );
    if ( now != NULL && ( now->tokenType == INT || now->tokenType == FLOAT ) )      
      return TrueNodeInit();
    return FalseNodeInit();
  } // Real()

  Tree Number( Tree now ) {
    now = Evaluate( now, 2 );
    if ( now != NULL && ( now->tokenType == INT || now->tokenType == FLOAT ) )      
      return TrueNodeInit();
    return FalseNodeInit();
  } // Number()

  Tree Bool( Tree now ) {
    now = Evaluate( now, 2 );
    if ( now != NULL && ( now->tokenType == NIL || now->tokenType == T ) )      
      return TrueNodeInit();
    return FalseNodeInit();
  } // Bool()

  Tree String( Tree now ) {
    now = Evaluate( now, 2 );
    if ( now != NULL && now->tokenType == STR )      
      return TrueNodeInit();
    return FalseNodeInit();
  } // String()

  Tree Symbol( Tree now ) {
    now = Evaluate( now, 2 );
    if ( now != NULL && now->tokenType == SYM )      
      return TrueNodeInit();
    return FalseNodeInit();
  } // Symbol()

  Tree Atom( Tree now ) {
    now = Evaluate( now, 2 );
    if ( now != NULL && IsAtom( now ) )
      return TrueNodeInit();
    return FalseNodeInit();
  } // Atom()
  // 若結果為nil即true
  Tree Not( Tree now ) {
    now = Evaluate( now, 2 );
    if ( mhasError )
      return NULL;
    if ( now != NULL && now->tokenType == NIL )      
      return TrueNodeInit();
    return FalseNodeInit();
  } // Not()
  // + - * /
  Tree Arithmetic( string sym, string op, Tree now, int level ) {
    // sym是整句的sym op是要執行的function
    if ( now == NULL ) 
      return EvalError( 7, sym, now );
    Tree left = now->left, right = now->right, temp = CopyPointer( left );
    if ( left->nodeType == INTER || left->tokenType == SYM )
      left = Evaluate( left, level + 1 );
    // 檢查運算元型別
    if ( left == NULL ) return EvalError( 13, sym, temp );
    else if ( left->tokenType != INT && left->tokenType != FLOAT )
      return EvalError( 9, sym, left );
    if ( right == NULL ) return left;
    // 遞迴右邊並合併
    if ( op == "+" ) return Add( left, Arithmetic( sym, op, right, level ) );
    else if ( op == "-" ) return Sub( left, Arithmetic( sym, "+", right, level ) );
    else if ( op == "*" ) return Mul( left, Arithmetic( sym, op, right, level ) );
    else {
      right = Arithmetic( sym, "*", right, level );
      // 除數 0
      if ( right != NULL && right->value == 0 )
        return EvalError( 12, "/", now );
      return Div( left, right );
    } // else
  } // Arithmetic()

  Tree Add( Tree leftsub, Tree rightsub ) {
    Tree newNode;
    TokenInit( newNode );
    newNode = NumberType( newNode, leftsub, rightsub );
    if ( leftsub != NULL )
      newNode->value += leftsub->value;
    if ( rightsub != NULL )
      newNode->value += rightsub->value;
    return newNode;
  } // Add()

  Tree Sub( Tree leftsub, Tree rightsub ) {
    Tree newNode;
    TokenInit( newNode );
    newNode = NumberType( newNode, leftsub, rightsub );
    if ( leftsub != NULL ) {
      if ( rightsub != NULL )
        newNode->value = leftsub->value - rightsub->value;
      else
        newNode->value = leftsub->value;
    } // if
    else if ( rightsub != NULL )
      newNode->value = 0 - rightsub->value;
    else
      newNode->value = 0;
    return newNode;
  } // Sub()

  Tree Mul( Tree leftsub, Tree rightsub ) {
    Tree newNode = IntNodeInit( 1 );
    newNode = NumberType( newNode, leftsub, rightsub );
    if ( leftsub != NULL )
      newNode->value *= leftsub->value;
    if ( rightsub != NULL )
      newNode->value *= rightsub->value;
    return newNode;
  } // Mul()

  Tree Div( Tree leftsub, Tree rightsub ) {
    Tree newNode;
    TokenInit( newNode );
    newNode = NumberType( newNode, leftsub, rightsub );
    if ( leftsub != NULL ) {
      if ( rightsub != NULL && rightsub->value != 0 )
        newNode->value = leftsub->value / rightsub->value;
      else if ( rightsub == NULL )
        newNode->value = leftsub->value;
    } // if
    else if ( rightsub != NULL )
      newNode->value = rightsub->value;
    else
      newNode->value = 0;
    if ( newNode->tokenType == INT ) newNode->value = floor( newNode->value );
    return newNode;
  } // Div()

  Tree Compare( string sym, Tree now, int level ) {
    if ( now == NULL )
      return FalseNodeInit();
    // 檢查arg >= 2
    else if ( now->nodeType == LEAF || now->right == NULL ) {
      cout << "arg should bigger than 2" << endl;
      return EvalError( 7, sym, NULL );
    } // else if
    else 
      // inter
      now = CompareRecur( sym, now, level );
    if ( now != NULL && now->tokenType == NIL ) return FalseNodeInit() ;
    else return TrueNodeInit();
  } // Compare()

  Tree CompareRecur( string sym, Tree now, int level ) {
    // 回傳false || 待比的
    if ( now == NULL )
      return TrueNodeInit();
    else if ( now->left == NULL ) {
      cout << "unknown error occur" << endl;
      throw "error occur";
    } // else if

    Tree leftsub = now->left, rightsub = now->right;
    if ( leftsub->nodeType == INTER || leftsub->tokenType == SYM )
      leftsub = Evaluate( leftsub, level + 1 );
    // 型別錯
    if ( leftsub->tokenType != INT && leftsub->tokenType != FLOAT )
      return EvalError( 9, sym, leftsub ); 
    if ( rightsub == NULL )
      return leftsub;
    // 回傳false or 待比leaf
    rightsub = CompareRecur( sym, rightsub, level );
    if ( rightsub != NULL && rightsub->tokenType == NIL )
      return FalseNodeInit();
    if ( leftsub != NULL ) {
      if ( rightsub != NULL ) {
        if ( ( sym == ">" && leftsub->value > rightsub->value ) ||
             ( sym == "<" && leftsub->value < rightsub->value ) || 
             ( sym == "=" && leftsub->value == rightsub->value ) ||
             ( sym == ">=" && leftsub->value >= rightsub->value ) ||
             ( sym == "<=" && leftsub->value <= rightsub->value ) )
          return leftsub;
        else
          return FalseNodeInit();
      } // if
      else return leftsub;
    } // if

    return NULL;
  } // CompareRecur()

  Tree StrCompare( string sym, Tree now, int level ) {
    if ( now == NULL )
      return FalseNodeInit();
    else 
      // inter
      now = StrCompareRecur( sym, now, level );
    if ( now->tokenType == NIL ) now = FalseNodeInit();
    else now = TrueNodeInit();
    return now;
  } // StrCompare()

  Tree StrCompareRecur( string sym, Tree now, int level ) {
    Tree leftsub = now->left, rightsub = now->right;
    if ( leftsub->nodeType == INTER || leftsub->tokenType == SYM )
      leftsub = Evaluate( leftsub, level + 1 );
    if ( leftsub->tokenType != STR )
      return EvalError( 9, sym, leftsub ); 
    if ( rightsub == NULL )
      return leftsub;
      // 回傳false or 待比leaf
    rightsub = StrCompareRecur( sym, rightsub, level );
    if ( rightsub != NULL && rightsub->tokenType == NIL )
      return FalseNodeInit();
    if ( leftsub != NULL ) {
      if ( rightsub != NULL ) {
        if ( ( sym == "string>?" && leftsub->token > rightsub->token ) ||
             ( sym == "string<?" && leftsub->token < rightsub->token ) ||
             ( sym == "string=?" && leftsub->token == rightsub->token ) ||
             ( sym == "string>=?" && leftsub->token >= rightsub->token ) ||
             ( sym == "string<=?" && leftsub->token <= rightsub->token ) )
          return leftsub;
        else
          return FalseNodeInit();
      } // if
      else return leftsub;
    } // if

    return NULL;
  } // StrCompareRecur()

  Tree StrAppend( Tree now, int level ) {
    // 字串相接 不限arg數 但必為str type
    if ( now == NULL )
      return NULL;
    else if ( now->nodeType == LEAF ) {
      if ( now->tokenType == STR )
        return now;
      else {
        now = Evaluate( now, level + 1 );
        if ( now != NULL && now->tokenType != STR )
          return EvalError( 9, "string-append", now ); // op型別錯
        return now;
      } // else
    } // else if
    else {
      Tree leftsub = now->left, rightsub = now->right;
      if ( leftsub != NULL )
        leftsub = Evaluate( leftsub, level + 1 );
      if ( rightsub != NULL && rightsub->nodeType == INTER )
        // 回傳str
        rightsub = StrAppend( rightsub, level );
      if ( leftsub != NULL ) {
        if ( leftsub->tokenType == STR ) {
          if ( rightsub != NULL ) {
            if ( rightsub->tokenType == STR ) {
              leftsub->token = PopBack( leftsub->token );
              rightsub->token = rightsub->token.assign( rightsub->token, 1, rightsub->token.length() - 1 );
              leftsub->token = leftsub->token + rightsub->token;
            } // if
            else
              return EvalError( 9, "string-append", rightsub ); // op型別錯
          } // if
        } // if
        else
          return EvalError( 9, "string-append", leftsub ); // op型別錯
      } // if
      else if ( rightsub != NULL && rightsub->tokenType == STR )
        return rightsub;
      else 
        return EvalError( 9, "string-append", leftsub ); // op型別錯
      return leftsub;
    } // else
  } // StrAppend()
  // 型別要一樣、token要一樣(3 3)、(b a)...且只能是atom、'((3 (4 5))) '((3 (4 5)))不行、STR不行
  // 計算完後 指向同一塊記憶體||相等原子
  Tree Eqv( Tree now, int level ) {
    bool isEqv = IsEqv( now, level );
    if ( isEqv ) return TrueNodeInit();
    else return FalseNodeInit();
  } // Eqv()
  // 當兩者是相等原子||兩個被比較對象指向同一記憶體位址
  bool IsEqv( Tree now, int level ) {
    Tree leftsub = now->left, rightsub = now->right;
    if ( rightsub == NULL )
      return true;
    // 先算右邊
    if ( !IsEqv( rightsub, level ) ) return false;
    rightsub = rightsub->left;
    // 計算
    int index;
    leftsub = Evaluate( leftsub, level + 1 );
    rightsub = Evaluate( rightsub, level + 1 );
    // atom相等(字串例外)
    if ( IsAtom( leftsub ) && IsAtom( rightsub ) && leftsub->tokenType != STR && 
         leftsub->tokenType == rightsub->tokenType &&
         ( ( leftsub->token == rightsub->token ) || 
           ( ( leftsub->tokenType == INT || leftsub->tokenType == FLOAT ) && 
             leftsub->value == rightsub->value ) ) )
      return true;
    // SYM  記憶體位置相同
    else if ( leftsub == rightsub ) return true;
    return false;
  } // IsEqv()
  // 計算完後 值一樣
  Tree Equal( Tree now, int level ) {
    if ( EqualRecur( now, level ) ) return TrueNodeInit();
    else return FalseNodeInit();
    return now;
  } // Equal()

  bool EqualRecur( Tree now, int level ) {
    Tree leftsub = now->left, rightsub = now->right;
    if ( rightsub == NULL ) return true;
    // 右邊先算完
    if ( !EqualRecur( rightsub, level ) ) return false;
    else if ( leftsub != NULL ) {
      if ( rightsub != NULL ) {
        // 先各自計算
        rightsub = rightsub->left;
        leftsub = Evaluate( leftsub, level + 1 );
        rightsub = Evaluate( rightsub, level + 1 );
        // 比較是否相等
        if ( IsSameTree( leftsub, rightsub ) )
          return true;
      } // if
      else return true;
    } // else if

    return false;
  } // EqualRecur()
  // 除非回傳nil 否則一律true，條件沒限制型別，但arg一律3個
  Tree If( Tree now, int level ) {
    Tree condition = now->left, t = now->right->left, f = NULL;
    if ( now->right->right != NULL ) f = now->right->right->left;
    condition = Evaluate( condition, level + 1 );
    if ( mhasError || condition == NULL ) 
      return NULL;
    if ( condition->tokenType != NIL ) return Evaluate( t, level + 1 );
    else return Evaluate( f, level + 1 ) ;
  } // If()
  // 回傳第一個值是nil的人或最後一個值
  Tree And( Tree now, int level ) {
    Tree op1 = Evaluate( now->left, level + 1 ) ;
    if ( op1 == NULL )
      return NULL;
    if ( op1->tokenType == NIL || now->right == NULL ) return op1;
    else return And( now->right, level );
  } // And()
  // 回傳遞一個值不是nil的人或最後一個值
  Tree Or( Tree now, int level ) {
    Tree op1 = Evaluate( now->left, level + 1 ) ;
    if ( op1 == NULL )
      return NULL;
    if ( op1->tokenType != NIL || now->right == NULL ) return op1;
    else return Or( now->right, level );
  } // Or()
  // 不用先確認sym是否define 跑到那個條件再確認就好
  Tree Cond( Tree now, int level ) {
    // 若條件為sym，且為else，代表"任何其他案例" 不管binding
    // 只有當最後一個條件是else時，才是other case，否則為一般sym
    // 大於等於0個arg (cond (test1 action1) (test2 action2) ... )
    // action可以存在好幾個 印最後一個就好
    if ( now == NULL )
      return NULL;
    else if ( now->nodeType == LEAF )
      return EvalError( 9, "cond", now );
    Tree team = now->left, test = team->left, action = team->right;
    for ( Tree temp = team ; temp->right != NULL ; temp = temp->right ) 
      action = temp->right->left;
    if ( test->tokenType == SYM && test->token == "else" && now->right == NULL ) {
      if ( IsAllBound( team->right ) )  // 就算只印最後一個 也要檢查是否define
        return Evaluate( action, level + 1 );
      return NULL;
    } // if

    test = Evaluate( test, level + 1 );
    if ( test != NULL && test->tokenType != NIL ) {
      if ( IsAllBound( team->right ) )  // 就算只印最後一個 也要檢查是否define
        return Evaluate( action, level + 1 );
      return NULL;
    } // if
    else return Cond( now->right, level );
  } // Cond()
  // 回傳最後一個argu並展開
  Tree Begin( Tree now, int level ) {
    if ( now == NULL )
      return NULL;
    else if ( now->nodeType == LEAF )
      return Evaluate( now, level );
    else {
      // 未到底
      if ( now->right != NULL && now->right->nodeType == INTER ) {
        Evaluate( now->left, level + 1 );
        return Begin( now->right, level );
      } // if
      else
        // 展開
        return Evaluate( now->left, level + 1 );
    } // else
  } // Begin()
  // 0 有非Nil字元 / 1 正確
  bool Exit( Tree now, int level ) {
    // '(exit) / (exit nil) / (exit ( nil ) ) / ( exit . nil)
    if ( IsWhiteSpaceStr( now ) )
      return true;
    else if ( now->nodeType == LEAF && !IsWhiteSpaceStr( now ) ) 
      return false;
    bool left = Exit( now->left, level ) ;
    bool right = Exit( now->right, level );
    if ( left == false || right == false )
      return false;
    else
      return true;
  } // Exit()
  // 第一個argu是定義(格式:( SYMBOL S-expression )必須是兩個一對)，可以是空 ，此let結束要刪掉
  // 第二個(不可為空值)以後是計算(都要算，只取最後一個當值)
  Tree Let( Tree now, int level ) {
    vector<UserSymNode> temp;
    BackUp( mSym, temp );
    // 定義
    for ( Tree localDefine = now->left ; localDefine != NULL ; localDefine = localDefine->right ) {
      Tree pair = localDefine->left;
      if ( CheckArg( 2, pair ) && pair->left->tokenType == SYM ) 
        Define( mSym, true, pair->left->token, pair->right->left );
      else if ( !CheckArg( 0, pair ) && pair->tokenType != NIL ) return EvalError( 2, "let", pair );
    } // for
    // 計算
    Tree result = NULL ;
    for ( Tree remain = now->right ; remain != NULL ; remain = remain->right ) {
      Tree sexpr = remain->left;
      result = Evaluate( sexpr, level + 1 );
    } // for

    ReDuction( mSym, temp );
    return result;
  } // Let()
  // 第一個argu是用在後面的symbol(0~n個)
  // 第二個(不可為空值)以後是function(不用算)(只取最後一個)
  Tree Lambda(  Tree now, int level ) {
    // 檢查變數
    // DefineParameter( now->left, "lambda" );
    // 找最後一個s-expression
    Tree function = now->right;
    for ( Tree sexpr = now->right ; sexpr != NULL ; sexpr = sexpr->right )
      function = sexpr->left;
    now->right = function;
    Tree newTempSym;
    TokenInit( newTempSym );
    newTempSym->tokenType = SYM;
    newTempSym->token = "#<procedure lambda>";
    Define( mSym, false, "lambdaTemp", now );
    return newTempSym;
  } // Lambda()

  void PrintAllSymTable() {
    for ( int i = 0 ; i < mLocalVar.size() ; i++ ) {
      cout << mLocalVar[i].sym << endl;
      // cout << mSym[i].binding << endl;
      PrintRecur( mLocalVar[i].binding );
    } // for
  } // PrintAllSymTable()

public:
  Parser() {
    misEnd = false, mhasError = false;
    mroot = NULL;
  } // Parser()

  void ReadFile() {
    // 讀到(exit) or EOF為止
    cout << "Welcome to OurScheme!\n";
    while ( !misEnd ) {
      mLocalVar.clear();
      // mReduction = true;
      try {
        GetSexpr();
        if ( !mhasError && !misEnd ) {
          Tree resultSexpr = NULL, tempNoReturn = CopyPointer( mroot );
          resultSexpr = Evaluate( mroot, 1 );
          if ( resultSexpr == NULL ) 
            EvalError( 11, tempNoReturn->token, tempNoReturn ); // LAMBDA
          else if ( !mhasError && !misEnd )
            PrintRecur( resultSexpr );
        } // if
      } // try
      catch ( const char* message ) {
        ; // cout << message << endl;
      } // catch
      if ( misEnd && !mhasError )
        cout << endl;
      CleanAll();
    } // while

    cout << "Thanks for using OurScheme!";
  } // ReadFile()
};

int main() {
  char uTestNum, c;
  scanf( "%c%c", &uTestNum, &c ); // c是防錯用
  Parser parser;
  parser.ReadFile();
} // main()
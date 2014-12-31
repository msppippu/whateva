/*SooMin Cho's Own Scripting Language*****************************/
/*EEN322 Final Project*********************************12/14/2014*/


/* To be accomplished:--------------------------------------------/
1. Give variables new values, and access those variables
2. Perform simple arithmetic and various tests
     (such as +, /, <, >=, does file xxx exist, etc.)
3. Conditional execution of commands (e.g. if)
4. Repeated execution of commands (e.g. while)
5. Run other programs (as in first version of shell)
6. Access exit code from a program when it finishes
7. Execute built-in commands (as in first version of shell)
8. Substitute values for variables in commands 
/----------------------------------------------------------------*/


/*Design Plan:----------------------------------------------------/
1. Obtain source string from the user with my own shell
2. Call the lexical analyzer and create tokens
3. Call the syntatic Analyzer
	--> Create a parse tree
4. Compile, interpret, and translate elements from the parse tree
	--> Create a symbol table using hash for storing variables
5. Output 
/----------------------------------------------------------------*/



#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//global constants
#define maxinput 2048
#define uninitialized -999
#define inapplicable 999

//Enumerate and define tokens
typedef enum {IF, WHILE, SYMBOL, ASSIGN, ARITH, STRING, NUM, COMP, SPECIAL}tok_type;
typedef enum {ex_success=0, ex_fail=-1, ex_warn=-2}exit_code;


//global variables
int dont_read = 0;
exit_code exitted = 0;

void skip()
{
	dont_read=1;
}


/*token struct-------------------------------------------*/
typedef struct _token
{	
	tok_type type;	
	char *detail;
	double value;
}token;

//token constructor
token* new_token()
{
	token * tok= (token*) malloc(sizeof(token));
	tok->detail = (char*) malloc(256); //no detail will be longer than 10 characters
	tok->value = 0;
	return tok;
}

void set_token(token *tok, tok_type t, char* d, double v){
	tok->type=t;
	tok->detail=d;
	tok->value=v;
}

//token destructor
void free_token(token *tok)
{
	free(tok->detail);
	free(tok);
}

/*--------------------------------------------------------*/


/*struct for our input, which will be processed 
by the lexical analyzer and be broken down to tokens.
For this assignment, we are limiting input length to 2048 characters*/

typedef struct _input
{
	char *line; //the whole input string
	int len; //the length of the input (max index +1)
	int index; //the index 
}input;

//Constructor for input
input* new_input(char *s){
	input* in =(input*) malloc(sizeof(input));
	in->len = strlen(s)+1;
	in->line = (char*)  malloc(in->len);
	in->line = s;
	in->index = 0;
}

//Destructor for input
void free_input(input * in){
	free(in->line);
	free(in);
}

//For accessing the next character
char next_char(input *in)
{	
	if (in->index >= in->len) return '\0';
	else 
	{	(in->index)++;
		int i=in->index -1;
		return in->line[i];
	}
}

void prev_char(input *in)
{
	if(in->index >= in->len||in->index<=0)
	{
		return;
	}
	else 
	{
		(in->index)--;
	}
	
}

//returns 1 if any whitespace skipped, 0 if not
int skip_ws(input *in)
{
	int i = in->index;
	while (in->line[in->index]==' '&& in->index < in->len )
		in->index ++;
	if (i < in->index) return 1;
	else return 0;
	
}

/*Lexical Analyzer:
A function that returns the first recognized token
and moves the input index*/


/*Some funny rules...
--Rule 1: All elements must be separated by at least one space.
			i.e. 1+1 (X), 1 + 1 (O), 1    + 1 (O), 1 +1 (X)
--Rule 2: All variables has to be named starting with @.
			i.e. x = 3 (X), @x = 3 (O), @str = "string var" (0)
*/
token* lexAn(input *in)
{	
	if(dont_read)
	{
		dont_read=0;
		return NULL;
	}
	
	token *tok = new_token();

	char c = next_char(in);

	if(isdigit(c)) //if it starts with a digit, loop till found a NON-digit
	{	
		char *s=(char *)malloc(11); //unsigned int max is 10 digits long: 4,294,967,295
		int i=0;
		while(isdigit(c))
		{
			s[i]=c;
			i++;
			c = next_char(in);
		}
		if(c != ' '&&c != '\n') 
		{
			printf("ERROR: Put space after integer\n");
			exitted = -1;
			return NULL;
		}//if not separated by space, error code is on
		s[i]='\0';
		tok->type = NUM;
		tok->detail = s;
		free(s);
		tok->value = atoi(tok->detail);
		return tok;
	}
	else if (c == '\"')//STRING if it starts with a semicolon
	{
		c = next_char(in); //look at the next char
		char *s=(char *)malloc(256);
		int i=0;
		while(c!='\"'&& i<256)
		{	
			s[i]=c;
			i++;
			c = next_char (in);
		}
		if (c != '\"')
		{
			printf("WARNING: String too long\n");
			exitted = ex_warn;
		}
		s[i]='\0';
		tok->type = STRING;
		tok->detail = s;
		free(s);
		tok->value = inapplicable; //need to decide what value i want to give strings
		return tok;
	}
	else if (c == '@')
	{
		c=next_char (in);
		if (!isalpha(c))
		{
			printf("ERROR: Variable should start with an alphabet letter followed by '@'\n");
			exitted=ex_fail;
			return NULL;
		}
		char *s= (char *)malloc(20); //no variable is more than 20 chars long
		int i=0;
		while (c!=' '&&i < 20)
		{
			s[i]=c;
			i++;
			c = next_char(in);
		}
		s[i]='\0';
		tok->type = SYMBOL;
		tok->detail = s;
		free(s);
		tok->value = uninitialized;
		return tok;
	}
	else if (c == '=')
	{
		c=next_char (in);
		if (c == '='){
			char *s = (char *) malloc (3);
			strlcpy(s, "==", 3);
			tok->type=COMP;
			tok->detail=s;
			free(s);
			tok->value=inapplicable;
			return tok;
		}
		if (c != ' ')
		{
			printf("ERROR: '=' should be separated by space\n");
			exitted=ex_fail;
			return NULL;
		}
		char *s=(char *)malloc(2);
		s[0]='=';
		s[1]='\0';
		tok->type=ASSIGN;
		tok->detail=s;
		free(s);
		tok->value=inapplicable;
		return tok;
	}
	else if (c=='<')
	{
		c=next_char (in);
		if (c==' '){
			char *s = (char *)malloc(2);
			s[0]='<';
			s[1]='\0';
			tok->type=COMP;
			tok->detail=s;
			free(s);
			tok->value=inapplicable;
			return tok;
		}
		if(c=='='){
			c=next_char(in);
			if(c!= ' ')
			{
				printf("ERROR: '<=' should be separated by space\n");
				exitted=ex_fail;
				return NULL;
			}
			char *s = (char *)malloc(3);
			s[0]='<';
			s[1]='=';
			s[2]='\0';
			tok->type=COMP;
			tok->detail=s;
			free(s);
			tok->value=inapplicable;
			return tok;
		}
		printf("ERROR: '<' should be separated by space or followed by '='\n");
		exitted=ex_fail;
		return NULL;
	}
	else if (c=='>')
	{
		c=next_char (in);
		if (c==' '){
			char *s = (char *)malloc(2);
			s[0]='>';
			s[1]='\0';
			tok->type=COMP;
			tok->detail=s;
			free(s);
			tok->value=inapplicable;
			return tok;
		}
		if(c=='='){
			c=next_char(in);
			if(c!= ' ')
			{
				printf("ERROR: '<=' should be separated by space\n");
				exitted=ex_fail;
				return NULL;
			}
			char *s = (char *)malloc(3);
			s[0]='>';
			s[1]='=';
			s[2]='\0';
			tok->type=COMP;
			tok->detail=s;
			free(s);
			tok->value=inapplicable;
			return tok;
		}
		printf("ERROR: '>' should be separated by space or followed by '='\n");
		exitted=ex_fail;
		return NULL;
	}
	else if (c=='+'||c=='-'||c=='*'||c=='/')
	{
		char *s = (char*) malloc(2);
		s[0]=c;
		c=next_char(in);
		if (c!=' '){
			printf("ERROR: arithmetic operation should be separated by space\n");
			exitted=ex_fail;
			free(s);
			return NULL;
		}
		s[1]=c;
		tok->type=ARITH;
		tok->detail=s;
		tok->value=inapplicable;
		return tok;
	}
	else if (c=='i')
	{	
		char * s = (char *)malloc(3);
		c=next_char(in);
		if(c!='f')
		{
			printf("ERROR: unidentifiable token starting with 'i'\n");
			exitted=ex_fail;
			free(s);
			return NULL;
		}
		c=next_char(in);
		strlcpy(s, "if", 3);
		tok->detail=s;
		tok->type=IF;
		tok->value=inapplicable;
		return tok;
	}
	else if(c=='w')
	{
		char*s =(char*)malloc(6);
		s[0]=c;
		s[1]=next_char(in);
		s[2]=next_char(in);
		s[3]=next_char(in);
		s[4]=next_char(in);
		s[5]='\0';
		if(strcmp(s, "while")!=0)
		{
			printf("ERROR: unidentifiable token starting with 'w'\n");
			exitted=ex_fail;
			free(s);
			return NULL;
		}
		tok->detail=s;
		tok->type=WHILE;
		tok->value=inapplicable;
		return tok;

	}
	else
	{
		printf("ERROR: unidentifiable token\n");
		exitted=ex_fail;
		return NULL;
	}
	skip_ws(in);
}

/*node structs for Parser---------------------------------*/

//declare node union
union _node;
typedef union _node node;

typedef struct _myIff
{
	tok_type type;
	node * condition;
	node * body;
}myIff;

typedef struct _myWhi
{
	tok_type type;
	node * condition;
	node * body;

}myWhi;

typedef struct _myInt
{
	tok_type type;
	int val;
}myInt;

typedef struct _myStr
{
	tok_type type;
	char *val;
}myStr;

typedef struct _mySym
{
	tok_type type;
	int val;
}mySym;

typedef struct _myAsn
{
	tok_type type;
	node *sym;
	node *val;
}myAsn;

//Arithmetic
typedef struct _myAri
{
	tok_type type;
	char * detail;
	node * a;
	node * b;
}myAri;

//echo
typedef struct _myEch
{
	tok_type type;
	char * detail;
	char *str;
}myEch;

//Compare
typedef struct _myCom
{
	tok_type type;
	char * detail;
	node *a;
	node *b;
}myCom;

/*-----------------------------------------------------------*/

//Define node union
union _node
{	
	myIff _if;
	myWhi _while;
	myInt _int;
	myStr _string;
	myAsn _assign;
	myAri _ari;
	myEch _echo;
	myCom _compare;
	tok_type type;
};


/*Parsing Tree:-----------------------------------------------/
/Should be able to store nodes in a certain order given by ---/
/grammar------------------------------------------------------/
/Literals have the lowest priority---------------------------*/

node * make_int(token *t)
{
	if(t->type == NUM)
	{
		myInt *n =  malloc (sizeof(myInt));
		n->type = NUM;
		n->val = (int) t->value;
		return (node *) n;
	}
}

node * make_string(token *t)
{
	if(t->type == STRING)
	{
		myStr *n = malloc(sizeof(myStr));
		n->type = STRING;
		strlcpy(n->val, t->detail, 256);
		return (node* ) n;
	}
}

node * make_symbol(token *t)
{
	if(t->type == SYMBOL)
	{
		mySym *n = malloc(sizeof(mySym));
		n->type = SYMBOL;
		n->val = (int)t->value;
		return (node *) n;
	}
}

node * make_node(token *t, node *lc, node *rc)
{
	if (t->type == ASSIGN)
	{
		myAsn *n = malloc(sizeof(myAsn));
		n->type = ASSIGN;
		n->sym = lc;
		n->val = rc;
		return (node *) n;
	}
	else if (t->type == COMP)
	{
		myCom *n = malloc (sizeof(myCom));
		n->type = COMP;
		n->a = lc;
		n->b = rc;
		return (node *) n;
	}
	else if (t->type == WHILE)
	{
		myWhi *n = malloc (sizeof(myWhi));
		n->type = WHILE;
		n->condition = lc;
		n->body = rc;
		return (node*) n;
	}
	else if (t->type == IF)
	{
		myIff *n = malloc (sizeof(myIff));
		n->type = IF;
		n->condition = lc;
		n->body = rc;
		return (node*) n;
	}
}

node * make_assignment(input *in)
{
	token *tok1 = new_token();
	tok1=lexAn(in);
	if(tok1->type != SYMBOL)
	{
		printf("ERROR: Incomplete assignment statement!\n");
		exitted=ex_fail;
		return NULL;
	}
	node *lc = make_symbol(tok1);
	free(tok1);

	token *tok2 = new_token();
	tok2 = lexAn(in);
	if (tok2->type != ASSIGN)
	{
		printf("ERROR: Incomplete assignment statement!\n");
		exitted=ex_fail;
		return NULL;
	}
	
	token *tok3 = new_token();
	tok3 = lexAn(in);
	if(tok3->type != NUM)
	{
		printf ("ERROR: Incomplete assignment statement!\n");
		exitted=ex_fail;
		return NULL;
	}
	node *rc = make_int(tok3);
	free(tok3);

	return make_node(tok2, lc, rc);
}

//make comparison
node * make_comparison(input *in)
{
	token *tok1 = new_token();
	tok1=lexAn(in);
	node *lc = malloc(sizeof(node));
	node *rc = malloc(sizeof(node));
	if(tok1->type == SYMBOL)
	{
		node *lc = make_symbol(tok1);
	} 
	else if(tok1->type == NUM)
	{
		node *lc = make_int(tok1);
	}	
	else
	{
		printf("ERROR: Incomplete comparison statement!\n");
		exitted=ex_fail;
		free(lc);
		free(rc);
		return NULL;
	}
	free(tok1);

	token *tok2 = new_token();
	tok2 = lexAn(in);
	if (tok2->type != COMP)
	{
		printf("ERROR: Incomplete comparison statement!\n");
		exitted=ex_fail;
		return NULL;
	}
	
	token *tok3 = new_token();
	tok3 = lexAn(in);
	if(tok3->type == SYMBOL)
	{
		node *rc = make_symbol(tok3);
	} 
	else if(tok3->type == NUM)
	{
		node *rc = make_int(tok3);
	}	
	else
	{
		printf("ERROR: Incomplete comparison statement!\n");
		free(lc);
		free(rc);
		exitted=ex_fail;
		return NULL;
	}
	free(tok3);

	return make_node(tok2, lc, rc);
}

//make statement

//make expression

//make while

//make if

//etc...


/*-----------------------------------------------------------*/

/*Symbol Table------------------------------------------------/
/-Due to lack of time, I'm going to implement a Linked List---/
/-In real life, a linked list is ineffective because it takes-/
/-a long time to find a symbol this way, and when you have ---/
/-many symbols it will be highly time-consuming...-----------*/



typedef struct _lnode{
	char * name;
	int value;
	struct _lnode *next;
}lnode;

lnode * new_lnode(){
	lnode *n=(lnode*) malloc(sizeof(lnode));
	n->name = (char*) malloc(20);
	n->value = uninitialized;
	n->next = (lnode*) malloc(sizeof(lnode));
	return n;
}

typedef struct _llist{
	lnode *root;
	lnode *index;
	int count;
}llist;

llist *new_llist(){
	llist *l=(llist*) malloc(sizeof(llist));
	l->root=new_lnode();
	l->index=new_lnode();
	l->count=0;
	return l;
}

void add_lnode(char *name, int value, llist *l){
	lnode *n = new_lnode();
	n->name = name;
	n->value = value;
	if(l->count == 0)
	{
		l->root = n;
		l->index = n;
	}	
	else
	{
		l->index->next = n;
		l->index = n;
		l->count++;
	}
}
/*---------------------------------------------------------*/

int main(){
  char *s = (char *) malloc (maxinput);
  printf("Welcome to Soo's Shell..\n Hope you are generous!\n");

  while(1)
  {
	  printf("S++ Shell --> ");
	  fflush(stdout);
	  read(0, s, maxinput);
	  input *in= new_input(s);
	  make_comparison(in);

  }
  free(s);
  return 0;
}

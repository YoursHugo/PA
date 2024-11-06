/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>

enum {
  TK_NOTYPE = 256, 
  TK_EQ,
  TK_PLUS, 
  TK_SUB, 
  TK_MUL, 
  TK_DIV, 
  TK_NUM, 
  TK_LP, 
  TK_RP, 
  TK_VAB,
  TK_REG,
  /* TODO: Add more token types */

};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces
  {"\\+", TK_PLUS},         // plus
  {"==", TK_EQ},        // equal
  {"[-]?[0-9]+", TK_NUM},  // number
  {"-", TK_SUB},         // sub
  {"\\*", TK_MUL},
  {"/", TK_DIV},
  {"\\(", TK_LP},
  {"\\)", TK_RP},
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

static Token tokens[32] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      //printf("e[position] = %c\n", e[position]);//debug
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;
        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */
        switch (rules[i].token_type) {
          case TK_NOTYPE: break;
          case TK_PLUS:
          case TK_SUB:
          case TK_MUL:
          case TK_DIV:
          case TK_NUM: 
          case TK_EQ:
          case TK_LP:
          case TK_RP:
          case TK_VAB:
            tokens[nr_token].type = rules[i].token_type;
            strncpy(tokens[nr_token].str, substr_start, substr_len);
            break;
          default: assert(0);
        }
        if(rules[i].token_type != TK_NOTYPE) {
          nr_token++;
        }
        break;
      }
    }
    
    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

bool check_parentheses(int p, int q) {
  if (tokens[p].type != TK_LP || tokens[q].type != TK_RP) {
    return false;
  }
  int i, cnt = 0;
  for (i = p; i <= q; i++) {
    if (tokens[i].type == TK_LP) {
      cnt++;
    }
    else if (tokens[i].type == TK_RP) {
      cnt--;
    }
    if (cnt == 0 && i != q) {
      return false;
    }
  }
  return true;
}

u_int32_t eval(int p,int q) {
  if (p > q) {
    /* Bad expression */
    assert(0);
  }
  else if (p == q) {
    /* Single token.
     * For now this token should be a number.
     * Return the value of the number.
     */
    return atoi(tokens[p].str);
  }
  else if (check_parentheses(p, q) == true) {
    /* The expression is surrounded by a matched pair of parentheses.
     * If that is the case, just throw away the parentheses.
     */
    return eval(p + 1, q - 1);
  }
  else {
    int op= -1;//= the position of 主运算符 in the token expression;
    for(int i=p;i<=q;i++){
      if(tokens[i].type == TK_PLUS || tokens[i].type == TK_SUB || tokens[i].type == TK_MUL || tokens[i].type == TK_DIV ){
        op = i;
        break;
      }
    }
    u_int32_t val1 = eval(p, op - 1);
    u_int32_t val2 = eval(op + 1, q);

    switch (tokens[op].type) {
      case '+': return val1 + val2;
      case '-': return val1 - val2;
      case '*': return val1 * val2;
      case '/': return val1 / val2;
      default: assert(0);
    }
  }
}

word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }
  /* TODO: Insert codes to evaluate the expression. */
  //TODO();
  u_int32_t result = eval(0, nr_token-1);
  *success = true;
  return result;
}

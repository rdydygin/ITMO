#include "snake.h"
#include <assert.h>
#include <stdio.h>
static void basic(void) {
 SnakeGame g; snake_init(&g, 1);
 assert(g.length == 3 && g.status == SNAKE_RUNNING);
 for (unsigned i=0;i<g.length;i++) assert(g.food.x!=g.body[i].x || g.food.y!=g.body[i].y);
 SnakeCell head=g.body[0]; snake_turn(&g,SNAKE_LEFT); assert(g.direction==SNAKE_RIGHT);
 snake_turn(&g,SNAKE_UP); snake_turn(&g,SNAKE_LEFT); assert(g.direction==SNAKE_UP);
 snake_step(&g); assert(g.body[0].y==head.y-1);
 snake_pause(&g); head=g.body[0]; snake_step(&g); assert(g.body[0].y==head.y);
 snake_pause(&g); assert(g.status==SNAKE_RUNNING);
 snake_init(&g, 5); g.food=(SnakeCell){g.body[0].x+1,g.body[0].y};
 snake_step(&g); assert(g.length==4 && g.score==1);
 for(unsigned i=0;i<g.length;i++) assert(g.food.x!=g.body[i].x || g.food.y!=g.body[i].y);
 g.body[0].x=SNAKE_COLS-1; snake_step(&g); assert(g.status==SNAKE_LOST);
 snake_init(&g,0); assert(g.status==SNAKE_RUNNING && g.score==0);
 g.score=357; assert(snake_period_ms(&g)==90);
}
static void collisions(void) {
 SnakeGame g; snake_init(&g,1); g.length=4;
 g.body[0]=(SnakeCell){1,1}; g.body[1]=(SnakeCell){1,2};
 g.body[2]=(SnakeCell){2,2}; g.body[3]=(SnakeCell){2,1};
 g.food=(SnakeCell){10,10}; snake_step(&g);
 assert(g.status==SNAKE_RUNNING && g.body[0].x==2); /* Vacating tail. */
 snake_init(&g,1); g.length=5;
 g.body[0]=(SnakeCell){1,1}; g.body[1]=(SnakeCell){1,2};
 g.body[2]=(SnakeCell){2,2}; g.body[3]=(SnakeCell){2,1}; g.body[4]=(SnakeCell){3,1};
 g.food=(SnakeCell){10,10}; snake_step(&g); assert(g.status==SNAKE_LOST);
}
static void victory(void) {
 SnakeGame g; snake_init(&g,1);
 /* All inner cells except (2,1) are occupied; borders remain free. */
 g.body[0]=(SnakeCell){1,1}; unsigned at=1;
 for(unsigned y=1;y<SNAKE_ROWS-1;y++) for(unsigned x=1;x<SNAKE_COLS-1;x++) {
  if(y==1 && (x==1 || x==2)) continue;
  g.body[at++]=(SnakeCell){x,y};
 }
 g.length=at; g.food=(SnakeCell){2,1}; snake_step(&g);
 assert(g.status==SNAKE_WON && g.length==(SNAKE_COLS-2)*(SNAKE_ROWS-2) && g.score==1);
}
static void inner_food(void) {
 for(unsigned seed=1;seed<1000;seed++) {
  SnakeGame g; snake_init(&g,seed);
  assert(g.food.x>0 && g.food.x<SNAKE_COLS-1 && g.food.y>0 && g.food.y<SNAKE_ROWS-1);
  g.food=(SnakeCell){g.body[0].x+1,g.body[0].y}; snake_step(&g);
  assert(g.food.x>0 && g.food.x<SNAKE_COLS-1 && g.food.y>0 && g.food.y<SNAKE_ROWS-1);
 }
}
int main(void) { basic(); collisions(); victory(); inner_food(); puts("Snake tests passed"); }

#include<stdio.h>
# define maxsize 10
int stack[maxsize],top=-1; 
void push();
void pop();
void display();
int main()
{
	int ch;
	do
	{
	    printf("\n---------------stack----------------");
	    printf("\nenter 1 to push");
	    printf("\nenter 2 to pop");
	    printf("\nenter 3 to display");
	    printf("\nenter 4 to exit");
	    printf("\nenter your choice");
	    scanf("%d",&ch);
	    switch(ch)
	    {
		    case 1:push();break;
		    case 2:pop();break;
		    case 3:display();break;
		    case 4:break;
		    daufalt:printf("\nyou have enter wrong choice");
		}
    }while(ch!=4);
}
void push()
{
    if(top==maxsize-1)
    {
	printf("\nstack overflow");
	}
	else
	{
		int n;
	    printf("\nenter element in stack");
		scanf("%d",&n);
		top++;
		stack[top]=n;    
    }
}	
void pop()
{
	if(top==-1)
    {
	printf("\nstack underflow");
	}
	else
	{
		printf("\nthe deleted element is\t%d",stack[top]);
		top--;
	}
}
void display()
{
	if(top==-1)
    {
	printf("\nstack underflow");
	}
	else
	{
		int i;
		for(i=0;i<=top;i++)
		{
			printf(" %d\t",stack[i]);
		}
	}
}

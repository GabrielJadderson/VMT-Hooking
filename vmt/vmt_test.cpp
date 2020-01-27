#include <stdio.h>
#include <windows.h>

struct fruit 
{
    int Calories;
    
    virtual void PrintCalories() 
    {    	
        printf("fruit->Calories: %d\n", Calories);
    }
    
    virtual void A() 
    {
        printf("A \n");
    }
    
    virtual void B() 
    {
        printf("B \n");
    }
};

struct apple : fruit
{
    void PrintCalories() override
    {
        printf("apple->Calories: %d\n", Calories);
    }
};

unsigned long long VMTLength(void** VMT) {
    unsigned long long i = 0;
	for (; VMT[i] && VMT[i] < (void *)VMT; i++);
	return i;
}

typedef void(__thiscall* VirtualFunction001)(void* ThisPtr);
VirtualFunction001 GlobalVirtualFunction001 = 0;

//as specified by https://docs.microsoft.com/en-us/cpp/cpp/fastcall?view=vs-2019
//"The first two DWORD or smaller arguments that are found in the argument list
//from left to right are passed in ECX and EDX registers;
//all other arguments are passed on the stack from right to left."
void __fastcall OurHookFunction(void* ECXRegister, void* EDXRegister)
{
    //that means that the pointer to which "this" operator points to is now stored in the ECS register.
    //and we can retrieve it like so:
    fruit* FruitPtr = (fruit*)ECXRegister;
    //FruitPtr->PrintCalories(); //infinite recursion!
    FruitPtr->A();
    FruitPtr->B();

    printf("Broccoli is not a fruit but a malicious vegetable!\n");

	//Call the original 
	GlobalVirtualFunction001(ECXRegister);
}

void __fastcall VMTHookFnc(void* pEcx, void* pEdx)
{
    fruit* pThisPtr = (fruit*)pEcx;

    printf("In VMTHookFnc\n");
}

void Test(fruit* fruit)
{
    fruit->PrintCalories();
    fruit->A();
    fruit->B();
}


int main() 
{
    fruit MyFruit;
    MyFruit.Calories = 0;

    apple MyApple;
    MyApple.Calories = 52;

    fruit* AllocatedAppleFruit = new apple;
    AllocatedAppleFruit->Calories = 42;
    
    Test(&MyFruit);

    printf("\n\n");

  
    void** VirtualFunctionTable = *(void***)&MyFruit;
    unsigned long long NumberOfVirtualFunctions = VMTLength(VirtualFunctionTable);

    //retrieving the functions:
	void* FuncEntry1 = VirtualFunctionTable[0];
	void* FuncEntry2 = VirtualFunctionTable[1];
    void* FuncEntry3 = VirtualFunctionTable[2];

    //Call FuncEntry1 directly
    GlobalVirtualFunction001 = (VirtualFunction001)FuncEntry1;
    GlobalVirtualFunction001(&MyFruit); //we'll have to pass in MyFruit such that we have access to MyFruit.Calories.

    ((void (*)(void))FuncEntry2)(); //we know it returns void and takes no arguments.

	
    //begin hook
    DWORD OldProtectionFlags = 0;
    VirtualProtect(&VirtualFunctionTable[0], sizeof(void*), PAGE_EXECUTE_READWRITE, &OldProtectionFlags);

	//save the original function
    GlobalVirtualFunction001 = (VirtualFunction001)VirtualFunctionTable[0];

	//overwrite with our own function.
    VirtualFunctionTable[0] = &OurHookFunction;
    //memcpy(VirtualFunctionTable[0], &OurHookFunction, sizeof(void*));
    //WriteProcessMemory if we're hooking outside our address space.
 
	//Restore the original page protection
    VirtualProtect(&VirtualFunctionTable[0], sizeof(void*), OldProtectionFlags, &OldProtectionFlags);
    
    Test(&MyFruit);
    Test(&MyApple);
    Test(AllocatedAppleFruit);
    
    return 0;
}

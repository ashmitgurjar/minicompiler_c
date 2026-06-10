// Test all enhanced features of Mini-C

// 1. All Types & Initializers
int a = 10;
float b = 3.14;
bool c = true;
string d = "Hello, Mini-C World!\n";

print d;

// 2. Arithmetic & Coercion
float sum = a + b; // int 'a' coerced to float!
print "Sum (int + float) =";
print sum;

// 3. Relational & Logical Operators
bool check1 = a > 5;
bool check2 = b <= 4.0;
bool check3 = check1 && !check2;

print "Check1 (a > 5):";
print check1;
print "Check2 (b <= 4.0):";
print check2;
print "Check3 (check1 && !check2):";
print check3;

// 4. If-Else Control Flow
if (a == 10 && c) {
  print "Inside IF: a is 10 and c is true!";
} else {
  print "Inside ELSE (should not happen)";
}

if (b != 3.14) {
  print "b is not 3.14 (should not happen)";
} else {
  print "Inside ELSE: b is indeed 3.14";
}

// 5. Nested Loops & Scopes
print "Counting down with a while loop:";
int count = 5;
while (count > 0) {
  print count;
  count = count - 1;
}

print "Nested blocks and scopes:";
int x = 100;
{
  int x = 200;
  print "Inner x (should be 200):";
  print x;
}
print "Outer x (should be 100):";
print x;

print "Done!";

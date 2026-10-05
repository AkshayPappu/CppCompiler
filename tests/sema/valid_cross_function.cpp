int foo() {
    int x = 5;
    int y = 10;
    return x + y;
}

int bar() {
    int x = 100;
    int y = 200;
    return x + y;
}

int main() {
    int x = foo();
    int y = bar();
    return x + y;
}

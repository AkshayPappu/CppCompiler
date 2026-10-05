int main() {
    int a = 1;
    int b = 2;
    int c = 3;

    /* right-associative assignment */
    a = b = 5;

    // precedence: a + (b * c)
    int d = a + b * c;

    // unary operators
    int e = -a;
    bool f = !true;

    // mixed precedence with parens
    int g = (a + b) * c;

    // logical with comparison
    bool h = a > 5 && b < 10 || c == 3;

    return 0;
}

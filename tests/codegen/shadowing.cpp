int main() {
    int x = 5;
    {
        int x = 10;
        x = x + 1;
    }
    return x;
}

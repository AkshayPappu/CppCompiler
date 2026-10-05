int main() {
    int x = 5;

    {
        int x = 10;
        int y = x + 1;
    }

    {
        bool x = true;
    }

    return x;
}

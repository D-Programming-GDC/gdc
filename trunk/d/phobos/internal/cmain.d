private extern (C) int _d_run_main(int argc, char **argv, void * p);
int main();
extern (C) int main(int argc, char **argv)
{
    return _d_run_main(argc, argv, & main);
}

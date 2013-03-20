void main() {
    version(GNU) asm {
        "unknown;" : : : ; // wrong opcode
    }
    else asm {
        unknown; // wrong opcode
    }
}


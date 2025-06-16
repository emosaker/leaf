/*
 * This file is part of the leaf programming language
 */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "compiler/bytecode.h"
#include "lib/array.h"
#include "debug/proto.h"

typedef struct lfLabel {
    int i;
    int label;
} lfLabel;

lfArray(lfLabel) mark_labels(const lfProto *proto) {
    lfArray(lfLabel) labels = array_new(lfLabel);
    int label = 0;
    for (int i = 0; i < proto->szcode; i++) {
        uint32_t ins = proto->code[i];
        switch (INS_OP(ins)) {
            case OP_JMPIFNOT:
            case OP_JMP: {
                bool found = false;
                for (int j = 0; j < length(&labels); j++) {
                    if (labels[j].i == i + INS_E(ins) / 4) {
                        found = true;
                        break;
                    }
                }
                if (found) {
                    break;
                }
                lfLabel lbl = (lfLabel) {
                    .i = i + INS_E(ins) / 4,
                    .label = label++
                };
                array_push(&labels, lbl);
            } break;
            case OP_JMPBACK: {
                bool found = false;
                for (int j = 0; j < length(&labels); j++) {
                    if (labels[j].i == i + INS_E(ins) / 4) {
                        found = true;
                        break;
                    }
                }
                if (found) {
                    break;
                }
                lfLabel lbl = (lfLabel) {
                    .i = i - INS_E(ins) / 4 - 1,
                    .label = label++
                };
                array_push(&labels, lbl);
            }
        }
    }
    return labels;
}

void print_string_comment(const char *string) {
    char buf[30];
    if (strlen(string) >= 26) {
        memcpy(buf, string, 26);
        buf[26] = '.';
        buf[27] = '.';
        buf[28] = '.';
        buf[29] = 0;
    } else {
        memcpy(buf, string, strlen(string));
        buf[strlen(string)] = '"';
        buf[strlen(string) + 1] = 0;
    }
    printf(" ; \"%s", buf);
}

void print_instruction(const lfProto *proto, lfArray(lfLabel) labels, int *i) {
    for (int j = 0; j < length(&labels); j++) {
        lfLabel lbl = labels[j];
        if (lbl.i == *i - 1) {
            printf(" .L%d:\n", lbl.label);
            break;
        }
    }
    if (*i >= proto->szcode) {
        (*i)++;
        return;
    }
    uint32_t *code = proto->code;
    uint32_t ins = code[*i];
    printf("  ");
    switch (INS_OP(ins)) {
        case OP_PUSHSI:
            printf("pushshort %u\n", INS_E(ins));
            break;
        case OP_PUSHLI:
            printf("pushlong %u (%lu)\n", INS_E(ins), proto->ints[INS_E(ins)]);
            break;
        case OP_PUSHS:
            printf("pushstring %u", INS_E(ins));
            print_string_comment(proto->strings[INS_E(ins)]);
            printf("\n");
            break;
        case OP_PUSHNULL:
            printf("pushnull\n");
            break;
        case OP_DUP:
            printf("dup %u\n", INS_E(ins));
            break;
        case OP_POP:
            printf("pop %d\n", INS_E(ins));
            break;

        case OP_GETGLOBAL:
            printf("getglob %u", INS_E(ins));
            print_string_comment(proto->strings[INS_E(ins)]);
            printf("\n");
            break;
        case OP_SETGLOBAL:
            printf("setglob %u", INS_E(ins));
            print_string_comment(proto->strings[INS_E(ins)]);
            printf("\n");
            break;
        case OP_GETUPVAL:
            printf("getupval %d\n", INS_E(ins));
            break;
        case OP_SETUPVAL:
            printf("setupval %d\n", INS_E(ins));
            break;
        case OP_INDEX:
            printf("index\n");
            break;
        case OP_ASSIGN:
            printf("assign\n");
            break;
        case OP_SET:
            printf("set\n");
            break;

        case OP_NEWARR:
            printf("newarr %u\n", INS_E(ins));
            break;
        case OP_NEWMAP:
            printf("newmap %u\n", INS_E(ins));
            break;

        case OP_ADD: printf("add\n"); break;
        case OP_SUB: printf("sub\n"); break;
        case OP_MUL: printf("mul\n"); break;
        case OP_DIV: printf("div\n"); break;
        case OP_POW: printf("pow\n"); break;
        case OP_EQ: printf("eq\n"); break;
        case OP_NE: printf("ne\n"); break;
        case OP_LT: printf("lt\n"); break;
        case OP_GT: printf("gt\n"); break;
        case OP_LE: printf("le\n"); break;
        case OP_GE: printf("ge\n"); break;
        case OP_BAND: printf("band\n"); break;
        case OP_BOR: printf("bor\n"); break;
        case OP_BXOR: printf("bxor\n"); break;
        case OP_BLSH: printf("blsh\n"); break;
        case OP_BRSH: printf("brsh\n"); break;
        case OP_AND: printf("band\n"); break;
        case OP_OR: printf("bor\n"); break;

        case OP_NEG: printf("neg\n"); break;
        case OP_NOT: printf("not\n"); break;

        case OP_JMP: {
            printf("jmp ");
            lfLabel lbl = (lfLabel) { .i = -1, .label = -1 };
            for (int j = 0; j < length(&labels); j++) {
                if (labels[j].i == *i + INS_E(ins) / 4)
                    lbl = labels[j];
            }
            printf(".L%d\n", lbl.label);
        } break;
        case OP_JMPBACK: {
            printf("jmp ");
            lfLabel lbl = (lfLabel) { .i = -1, .label = -1 };
            for (int j = 0; j < length(&labels); j++) {
                if (labels[j].i == *i - INS_E(ins) / 4)
                    lbl = labels[j];
            }
            printf(".L%d\n", lbl.label);
        } break;
        case OP_JMPIFNOT: {
            printf("jmpifnot ");
            lfLabel lbl = (lfLabel) { .i = -1, .label = -1 };
            for (int j = 0; j < length(&labels); j++) {
                if (labels[j].i == *i + INS_E(ins) / 4)
                    lbl = labels[j];
            }
            printf(".L%d\n", lbl.label);
        } break;

        case OP_CALL:
            printf("call args=%d, ret=%d\n", INS_A(ins), INS_B(ins));
            break;

        case OP_CL:
            printf("closure %u (%s)\n", INS_E(ins), proto->protos[INS_E(ins)]->name != 0 ? proto->protos[INS_E(ins)]->strings[proto->protos[INS_E(ins)]->name - 1] : "<anonymous>");
            break;
        case OP_CAPTURE:
            printf("capture %d (%s)\n", INS_D(ins), INS_A(ins) == UVT_REF ? "ref" : "idx");
            break;
        case OP_RET:
            printf("return %d\n", INS_A(ins));
            break;

        default:
            printf("unhandled: %d\n", INS_OP(ins));
    }
    *i += 1;
}

void lf_proto_print(const lfProto *proto) {
    for (int i = 0; i < proto->szprotos; i++) {
        lfProto *child = proto->protos[i];
        lf_proto_print(child);
    }
    if (proto->name != 0) {
        printf("%s:\n", proto->strings[proto->name - 1]);
    } else {
        printf("<anonymous>:\n");
    }
    int i = 0;
    lfArray(lfLabel) labels = mark_labels(proto);
    while (i < proto->szcode + 1) {
        print_instruction(proto, labels, &i);
    }
    array_delete(&labels);
}

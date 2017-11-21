#include <sys/alloc.h>
#include <sys/kprintf.h>
#include <sys/nary.h>
#include <sys/string.h>

struct nary_tree_node*
createNode(char* data)
{
    struct nary_tree_node* nary_node = kmalloc(sizeof(struct nary_tree_node));
    nary_node->data = kmalloc(strlen(data));
    strcpy(nary_node->data, data);
    nary_node->sibling = NULL;
    nary_node->firstChild = NULL;
    return nary_node;
}

void
calcPaths(char* path, char** subPath, char** remPath)
{
    char* slashPointOnwards = strchr(path, '/');
    if (slashPointOnwards != 0) {
        int subPathLength = (int)(strlen(path) - strlen(slashPointOnwards));
        *subPath = kmalloc(subPathLength + 1);
        strncpy(*subPath, path, subPathLength);
        *remPath = kmalloc(strlen(slashPointOnwards));
        *remPath = slashPointOnwards + 1;
    } else {
        *subPath = kmalloc(strlen(path));
        strcpy(*subPath, path);
        *remPath = NULL;
    }
}

void
insertInPath(struct nary_tree_node* root, char* path)
{
    char* subPath = NULL;
    char* remPath = NULL;
    while (1) {
        calcPaths(path, &subPath, &remPath);
        while (1) {
            if (strcmp(root->data, subPath) == 0) {
                if (root->firstChild == NULL) {
                    root->firstChild = createNode(remPath);
                    return;
                }
                root = root->firstChild;
                path = remPath;
                break;
            }
            if (root->sibling == NULL) {
                root->sibling = createNode(subPath);
                return;
            }
            root = root->sibling;
        }
    }
}

void
traverse(struct nary_tree_node* root, int tab)
{
    int td = tab;
    if (root != NULL) {
        while (td > 0) {
            kprintf("    ");
            td--;
        }
        kprintf("%s\n", root->data);
        traverse(root->firstChild, tab + 1);
        traverse(root->sibling, tab);
    }
}

void
insert(struct nary_tree_node** root, char* path)
{
    if (*root == NULL) {
        char* dot = ".";
        *root = createNode(dot);
        (*root)->firstChild = createNode(path);
        return;
    }
    insertInPath((*root)->firstChild, path);
}
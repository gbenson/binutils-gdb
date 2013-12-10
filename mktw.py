import os
import sys

root = os.path.dirname(os.path.realpath(sys.argv[0]))
lines = open(os.path.join(root, "libiberty", "cp-demangle.c")).readlines()

while True:
    line = lines.pop(0)
    if line.startswith("d_dump "):
        break

while True:
    line = lines.pop(0)
    if line.lstrip().startswith("switch "):
        break
assert lines[0].strip() == "{"
lines.pop(0)

class Caser:
    def __init__(self):
        self.__block = None
        self.branch_labels = {}

    def push_block(self):
        if self.__block is not None:
            self.__process(self.__block)
        self.__block = []

    def push_line(self, line):
        if line:
            self.__block.append(line)

    def __process(self, lines):
        line = lines.pop(0)
        PREFIX, SUFFIX = "case ", ":"
        assert line.startswith(PREFIX) and line.endswith(SUFFIX)
        label = line[len(PREFIX):-len(SUFFIX)]
        line = lines.pop()
        falls_through = {"break;": True, "return;": False}[line]
        body, start, branches = "\n".join(lines), 0, []
        while body:
            start = body.find("d_dump", start)
            if start == -1:
                break
            start = body.find("(", start)
            assert start > 0
            start += 1
            limit = body.find(",", start)
            assert limit > start
            branch = body[start:limit]
            branches.append(
                {"dc->u.s_binary.left": "d_left (dc)",
                 "dc->u.s_binary.right": "d_right (dc)"}.get(branch, branch))
            start = limit + 1
        if falls_through:
            branches.extend(("d_left (dc)", "d_right (dc)"))
        branches = tuple(branches)
        if not self.branch_labels.has_key(branches):
            self.branch_labels[branches] = []
        self.branch_labels[branches].append(label)

caser = Caser()
for line in lines:
    line = line.strip()
    if line == "}":
        break
    if line.startswith("case "):
        caser.push_block()
    caser.push_line(line)
caser.push_block()

order = {}
for line in open(os.path.join(root, "include", "demangle.h")).xreadlines():
    line = line.strip()
    if not line.startswith("DEMANGLE_COMPONENT_"):
        continue
    label = line.rstrip(",")
    assert not order.has_key(label)
    order[label] = len(order)

branch_labels = caser.branch_labels.items()
branch_labels.sort()
for branches, labels in branch_labels:
    labels = [(order[label], label) for label in labels]
    labels.sort()
    for tag, label in labels:
        print "    case %s:" % label
    for branch in branches:
        print "      XXX_shizzle (%s);" % branch
    print "      break;"
    print

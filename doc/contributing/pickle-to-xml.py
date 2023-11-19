#!/usr/bin/python


# output xml format:
# <pages>
# <page url="xx"><prev url="yyy">zzz</prev><next url="hhh">lll</next><fragment>file.frag</fragment></page>
# ...
# </pages>

import codecs
import os
import pickle


def dump_pickles(out, dirname, filename, path):
    with open(os.path.join(dirname, filename), "r", encoding="utf-8") as f:
        data = pickle.load(f)
    with codecs.open(
        data["current_page_name"] + ".frag", mode="w", encoding="utf-8"
    ) as fragment_file:
        fragment_file.write(data["body"])

    out.write('  <page url="%s">\n' % path)
    out.write("    <fragment>%s.frag</fragment>\n" % data["current_page_name"])
    if data["prev"] is not None:
        out.write(
            '    <prev url="%s">%s</prev>\n'
            % (os.path.normpath(os.path.join(path, data["prev"]["link"])), data["prev"]["title"])
        )
    if data["next"] is not None:
        out.write(
            '    <next url="%s">%s</next>\n'
            % (os.path.normpath(os.path.join(path, data["next"]["link"])), data["next"]["title"])
        )
    out.write("  </page>\n")

    if data["next"] is not None:
        next_path = os.path.normpath(os.path.join(path, data["next"]["link"]))
        next_filename = os.path.basename(next_path) + ".fpickle"
        dump_pickles(out, dirname, next_filename, next_path)


import sys

sys.stdout.write("<pages>\n")
dump_pickles(sys.stdout, os.path.dirname(sys.argv[1]), os.path.basename(sys.argv[1]), "/")
sys.stdout.write("</pages>")

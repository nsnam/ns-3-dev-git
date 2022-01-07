#!/usr/bin/env python3

import os
import subprocess
import tempfile
import sys
import filecmp
import optparse
import shutil
import difflib
import re

def git_modified_files():
    files = os.popen('git diff --name-only')
    process = subprocess.Popen(["git","rev-parse","--show-toplevel"],
                               stdout = subprocess.PIPE,
                               stderr = subprocess.PIPE)
    root_dir, _ = process.communicate()
    if isinstance(root_dir, bytes):
        root_dir=root_dir.decode("utf-8")
    files_changed = [item.strip() for item in files.readlines()]
    files_changed = [item for item in files_changed if item.endswith('.h') or item.endswith('.cc')]
    return [root_dir[: -1] + "/" + filename.strip () for filename in files_changed]

def copy_file(filename):
    _, pathname = tempfile.mkstemp()
    with open(filename, 'r') as src, open(pathname, 'w') as dst:
        for line in src:
            dst.write(line)
    return pathname

# generate a temporary configuration file
def uncrustify_config_file(level):
    level2 = """
nl_if_brace=Add
nl_brace_else=Add
nl_elseif_brace=Add
nl_else_brace=Add
nl_while_brace=Add
nl_do_brace=Add
nl_for_brace=Add
nl_brace_while=Add
nl_switch_brace=Add
nl_after_case=True
nl_namespace_brace=ignore
nl_after_brace_open=True
nl_class_leave_one_liners=False
nl_enum_leave_one_liners=False
nl_func_leave_one_liners=False
nl_if_leave_one_liners=False
nl_class_colon=Ignore
nl_before_access_spec=2
nl_after_access_spec=0
indent_access_spec=-indent_columns
nl_after_semicolon=True
pos_class_colon=Lead
pos_class_comma=Trail
indent_constr_colon=true
pos_bool=Lead
nl_class_init_args=Add
nl_template_class=Add
nl_class_brace=Add
# does not work very well
nl_func_type_name=Ignore
nl_func_scope_name=Ignore
nl_func_type_name_class=Ignore
nl_func_proto_type_name=Ignore
# function\\n(
nl_func_paren=Remove
nl_fdef_brace=Add
nl_struct_brace=Add
nl_enum_brace=Add
nl_union_brace=Add
mod_full_brace_do=Add
mod_full_brace_for=Add
mod_full_brace_if=Add
mod_full_brace_while=Add
mod_full_brace_for=Add
mod_remove_extra_semicolon=True
# max code width
#code_width=128
#ls_for_split_full=True
#ls_func_split_full=True
nl_cpp_lambda_leave_one_liners=True
"""
    level1 = """
# extra spaces here and there
sp_brace_typedef=Add
sp_enum_assign=Add
sp_before_sparen=Add
sp_after_semi_for=Add
sp_arith=Add
sp_assign=Add
sp_compare=Add
sp_func_class_paren=Add
sp_after_type=Add
sp_type_func=Add
sp_angle_paren=Add
"""
    level0 = """
sp_func_proto_paren=Add
sp_func_def_paren=Add
sp_func_call_paren=Add
sp_after_semi_for=Ignore
sp_before_sparen=Ignore
sp_before_ellipsis=Remove
sp_type_func=Ignore
sp_after_type=Ignore
nl_class_leave_one_liners=True
nl_enum_leave_one_liners=True
nl_func_leave_one_liners=True
nl_assign_leave_one_liners=True
nl_collapse_empty_body=True
nl_getset_leave_one_liners=True
nl_if_leave_one_liners=True
nl_fdef_brace=Ignore
# finally, indentation configuration
indent_with_tabs=0
indent_namespace=false
indent_columns=2
indent_brace=2
indent_case_brace=indent_columns
indent_class=true
indent_class_colon=True
indent_switch_case=indent_columns
# alignment
indent_align_assign=False
align_left_shift=True
# comment reformating disabled
cmt_reflow_mode=1 # do not touch comments at all
cmt_indent_multi=False # really, do not touch them
disable_processing_cmt= " *NS_CHECK_STYLE_OFF*"
enable_processing_cmt=  " *NS_CHECK_STYLE_ON*"
"""
    _, pathname = tempfile.mkstemp()
    with open(pathname, 'w') as dst:
        dst.write(level0)
        if level >= 1:
            dst.write(level1)
        if level >= 2:
            dst.write(level2)
    return pathname

## PatchChunkLine class
class PatchChunkLine:
    ## @var __type
    #  type
    ## @var __line
    #  line
    ## @var SRC
    #  Source
    SRC = 1
    ## @var DST
    #  Destination
    DST = 2
    ## @var BOTH
    #  Both
    BOTH = 3
    def __init__(self):
        """! Initializer
        @param self The current class
        """
        self.__type = 0
        self.__line = ''
    def set_src(self,line):
        """! Set source
        @param self The current class
        @param line source line
        @return none
        """
        self.__type = self.SRC
        self.__line = line
    def set_dst(self,line):
        """! Set destination
        @param self The current class
        @param line destination line
        @return none
        """
        self.__type = self.DST
        self.__line = line
    def set_both(self,line):
        """! Set both
        @param self The current class
        @param line
        @return none
        """
        self.__type = self.BOTH
        self.__line = line
    def append_to_line(self, s):
        """! Append to line
        @param self The current class
        @param s line to append
        @return none
        """
        self.__line = self.__line + s
    def line(self):
        """! Get line
        @param self The current class
        @return line
        """
        return self.__line
    def is_src(self):
        """! Is source
        @param self The current class
        @return true if type is source
        """
        return self.__type == self.SRC or self.__type == self.BOTH
    def is_dst(self):
        """! Is destination
        @param self The current class
        @return true if type is destination
        """
        return self.__type == self.DST or self.__type == self.BOTH
    def write(self, f):
        """! Write to file
        @param self The current class
        @param f file
        @return exception if invalid type
        """
        if self.__type == self.SRC:
            f.write('-%s\n' % self.__line)
        elif self.__type == self.DST:
            f.write('+%s\n' % self.__line)
        elif self.__type == self.BOTH:
            f.write(' %s\n' % self.__line)
        else:
            raise Exception('invalid patch')

## PatchChunk class
class PatchChunk:
    ## @var __lines
    #  list of lines
    ## @var __src_pos
    #  source position
    ## @var __dst_pos
    #  destination position
    ## @var src
    #  source
    ## @var dst
    #  destination
    def __init__(self, src_pos, dst_pos):
        """! Initializer
        @param self:  this object
        @param src_pos: source position
        @param dst_pos: destination position
        """
        self.__lines = []
        self.__src_pos = int(src_pos)
        self.__dst_pos = int(dst_pos)
    def src_start(self):
        """! Source start function
        @param self this object
        @return source position
        """
        return self.__src_pos
    def add_line(self,line):
        """! Add line function
        @param self The current class
        @param line line to add
        @return none
        """
        self.__lines.append(line)
    def src(self):
        """! Get source lines
        @param self The current class
        @return the source lines
        """
        src = []
        for line in self.__lines:
            if line.is_src():
                src.append(line)
        return src
    def dst(self):
        """! Get destination lines
        @param self The current class
        @return the destination lines
        """
        dst = []
        for line in self.__lines:
            if line.is_dst():
                dst.append(line)
        return dst
    def src_len(self):
        """! Get number of source lines
        @param self The current class
        @return number of source lines
        """
        return len(self.src())
    def dst_len(self):
        """! Get number of destinaton lines
        @param self The current class
        @return number of destination lines
        """
        return len(self.dst())
    def write(self,f):
        """! Write lines to file
        @param self The current class
        @param f: file to write to
        @return none
        """
        f.write('@@ -%d,%d +%d,%d @@\n' % (self.__src_pos, self.src_len(),
                                           self.__dst_pos, self.dst_len()))
        for line in self.__lines:
            line.write(f)

## Patch class
class Patch:
    ## @var __src
    #  source
    ## @var __dst
    #  destination
    ## @var __chunks
    #  chunks
    def __init__(self):
        """! Initializer
        @param self The current class
        """
        self.__src = ''
        self.__dst = ''
        self.__chunks = []
    def add_chunk(self, chunk):
        """! Add chunk
        @param self this object
        @param chunk chunk
        @return none
        """
        self.__chunks.append(chunk)
    def chunks(self):
        """! Get the chunks
        @param self The current class
        @return the chunks
        """
        return self.__chunks
    def set_src(self,src):
        """! Set source
        @param self this object
        @param src source
        @return none
        """
        self.__src = src
    def set_dst(self,dst):
        """! Set destination
        @param self this object
        @param dst destintion
        @return none
        """
        self.__dst = dst
    def apply(self,filename):
        """! Apply function
        @param self The current class
        @param filename file name
        @return none
        """
        # XXX: not implemented
        return
    def write(self,f):
        """! Write to file
        @param self The current class
        @param f the file
        @return none
        """
        f.write('--- %s\n' % self.__src )
        f.write('+++ %s\n' % self.__dst )
        for chunk in self.__chunks:
            chunk.write(f)

def parse_patchset(generator):
    src_file = re.compile('^--- (.*)$')
    dst_file = re.compile('^\+\+\+ (.*)$')
    chunk_start = re.compile('^@@ -([0-9]+),([0-9]+) \+([0-9]+),([0-9]+) @@')
    src = re.compile('^-(.*)$')
    dst = re.compile('^\+(.*)$')
    both = re.compile('^ (.*)$')
    patchset = []
    current_patch = None
    for line in generator:
        m = src_file.search(line)
        if m is not None:
            current_patch = Patch()
            patchset.append(current_patch)
            current_patch.set_src(m.group(1))
            continue
        m = dst_file.search(line)
        if m is not None:
            current_patch.set_dst(m.group(1))
            continue
        m = chunk_start.search(line)
        if m is not None:
            current_chunk = PatchChunk(m.group(1), m.group(3))
            current_patch.add_chunk(current_chunk)
            continue
        m = src.search(line)
        if m is not None:
            l = PatchChunkLine()
            l.set_src(m.group(1))
            current_chunk.add_line(l)
            continue
        m = dst.search(line)
        if m is not None:
            l = PatchChunkLine()
            l.set_dst(m.group(1))
            current_chunk.add_line(l)
            continue
        m = both.search(line)
        if m is not None:
            l = PatchChunkLine()
            l.set_both(m.group(1))
            current_chunk.add_line(l)
            continue
        raise Exception()
    return patchset

def remove_trailing_whitespace_changes(patch_generator):
    whitespace = re.compile('^(.*)([ \t]+)$')
    patchset = parse_patchset(patch_generator)
    for patch in patchset:
        for chunk in patch.chunks():
            src = chunk.src()
            dst = chunk.dst()
            try:
                for i in range(0,len(src)):
                    s = src[i]
                    d = dst[i]
                    m = whitespace.search(s.line())
                    if m is not None and m.group(1) == d.line():
                        d.append_to_line(m.group(2))
            except:
                return patchset
    return patchset


def indent(source, debug, level):
    output = tempfile.mkstemp()[1]
    # apply uncrustify
    cfg = uncrustify_config_file(level)
    if debug:
        sys.stderr.write('original file=' + source + '\n')
        sys.stderr.write('uncrustify config file=' + cfg + '\n')
        sys.stderr.write('temporary file=' + output + '\n')
    try:
        uncrust = subprocess.Popen(['uncrustify', '-c', cfg, '-f', source, '-o', output],
                                   stdin = subprocess.PIPE,
                                   stdout = subprocess.PIPE,
                                   stderr = subprocess.PIPE,
                                   universal_newlines = True)
        (out, err) = uncrust.communicate('')
        if debug:
            sys.stderr.write(out)
            sys.stderr.write(err)
    except OSError:
        raise Exception ('uncrustify not installed')
    # generate a diff file
    with open(source, 'r') as src, open(output, 'r') as dst:
        diff = difflib.unified_diff(src.readlines(), dst.readlines(),
                                    fromfile=source, tofile=output)
    if debug:
        initial_diff = tempfile.mkstemp()[1]
        sys.stderr.write('initial diff file=' + initial_diff + '\n')
        with open(initial_diff, 'w') as tmp:
            tmp.writelines(diff)
    final_diff = tempfile.mkstemp()[1]
    if level < 3:
        patchset = remove_trailing_whitespace_changes(diff)
        if len(patchset) != 0:
            with open(final_diff, 'w') as dst:
                patchset[0].write(dst)
    else:
        with open(final_diff, 'w') as dst:
            dst.writelines(diff)


    # apply diff file
    if debug:
        sys.stderr.write('final diff file=' + final_diff + '\n')
    shutil.copyfile(source,output)
    patch = subprocess.Popen(['patch', '-p1', '-i', final_diff, output],
                             stdin = subprocess.PIPE,
                             stdout = subprocess.PIPE,
                             stderr = subprocess.PIPE,
                             universal_newlines = True)
    (out, err) = patch.communicate('')
    if debug:
        sys.stderr.write(out)
        sys.stderr.write(err)
    return output



def indent_files(files, diff=False, debug=False, level=0, inplace=False):
    output = []
    for f in files:
        dst = indent(f, debug=debug, level=level)
        output.append([f,dst])

    # First, copy to inplace
    if inplace:
        for src,dst in output:
            shutil.copyfile(dst,src)
        return True

    # now, compare
    failed = []
    for src,dst in output:
        if filecmp.cmp(src,dst) == 0:
            failed.append([src, dst])
    if len(failed) > 0:
        if not diff:
            print('Found %u badly indented files:' % len(failed))
            for src,dst in failed:
                print('  ' + src)
        else:
            for src,dst in failed:
                with open(src, 'r') as f_src, open(dst, 'r') as f_dst:
                    s = f_src.readlines()
                    d = f_dst.readlines()
                    for line in difflib.unified_diff(s, d, fromfile=src, tofile=dst):
                        sys.stdout.write(line)
        return False
    return True

def run_as_main():
    parser = optparse.OptionParser()
    parser.add_option('--debug', action='store_true', dest='debug', default=False,
                      help='Output some debugging information')
    parser.add_option('-l', '--level', type='int', dest='level', default=0,
                      help="Level of style conformance: higher levels include all lower levels. "
                      "level=0: re-indent only. level=1: add extra spaces. level=2: insert extra newlines and "
                      "extra braces around single-line statements. level=3: remove all trailing spaces")
    parser.add_option('--check-git', action='store_true', dest='git', default=False,
                      help="Get the list of files to check from Git\'s list of modified and added files")
    parser.add_option('-f', '--check-file', action='store', dest='file', default='',
                      help="Check a single file")
    parser.add_option('--diff', action='store_true', dest='diff', default=False,
                      help="Generate a diff on stdout of the indented files")
    parser.add_option('-i', '--in-place', action='store_true', dest='in_place', default=False,
                      help="Indent the input files in-place")
    options, _ = parser.parse_args()
    style_is_correct = False

    if options.git:
        files = git_modified_files()
        style_is_correct = indent_files(files,
                                        diff=options.diff,
                                        debug=options.debug,
                                        level=options.level,
                                        inplace=options.in_place)
    elif options.file != '':
        file = options.file
        if not os.path.exists(file) or \
                not os.path.isfile(file):
            print('file %s does not exist' % file)
            sys.exit(1)
        style_is_correct = indent_files([file],
                                        diff=options.diff,
                                        debug=options.debug,
                                        level=options.level,
                                        inplace=options.in_place)

    if not style_is_correct:
        sys.exit(1)
    sys.exit(0)

if __name__ == '__main__':
     try:
        run_as_main()
     except Exception as e:
        sys.stderr.write(str(e) + '\n')
        sys.exit(1)

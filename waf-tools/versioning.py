## -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

import re

from waflib import ConfigSet, Configure, Context, Options, Task, TaskGen

CACHE_FILE = 'version.cache'

class ns3_version_info(Task.Task):
    '''Base task which implements functionality common to all inherited tasks

       This class handles parsing the ns-3 version tag into component parts
       as well as saving the version fields to a cache file

       All version fields should be stored in the fields property

       Derived classes should override _find_closest_tag() and 
       _find_closest_ns3_tag()
    '''

    def __init__(self, *args, **kwargs):
        self._fields = ConfigSet.ConfigSet()

        super(ns3_version_info, self).__init__(*args, **kwargs)

    @property
    def fields(self):
        return self._fields

    def _find_closest_tag(self, ctx):
        """Override in derived classes"""
        pass

    def _find_closest_ns3_tag(self, ctx):
        """Override in derived classes"""
        pass

    def _parse_version_tag(self, ctx, tag):
        safe_tag = tag.strip()
        matches = re.match("ns-(\d+)\.(\d+)(?:.(\d+))?(?:-(RC.+))?.*", safe_tag)
          
        if not matches:
            return False

        self.fields['VERSION_TAG'] = '"{}"'.format(safe_tag)
        self.fields['VERSION_MAJOR'] = matches.group(1)
        self.fields['VERSION_MINOR'] = matches.group(2)

        patch = matches.group(3)

        if not patch:
            patch = '0'

        self.fields['VERSION_PATCH'] = patch 

        release_candidate = matches.group(4)

        if not release_candidate:
            release_candidate = ''

        self.fields['VERSION_RELEASE_CANDIDATE'] = '"{}"'.format(release_candidate)

        return True

    def run(self):
        ctx = self.generator.bld

        try:
            self._find_closest_tag(ctx)

            if 'VERSION_TAG' not in self.fields:
                self._find_closest_ns3_tag(ctx)

            #Generate the path where the cache file will be stored
            base_path = self.generator.path.make_node('model')
            cache_path = base_path.make_node(CACHE_FILE)

            #Write the version information out to the cache file 
            #The cache file is used to populate the version fields when a git 
            #repository can not be found 
            self.fields.store(cache_path.abspath())

            #merge version info into main configset
            ctx.env.update(self.fields)

        except Exception as e:
            ctx.to_log("Extracting version information from tags failed: {}\n".format(e)) 
            return 1
    
        return 0

class git_ns3_version_info(ns3_version_info):
    '''Task to generate version fields from an ns-3 git repository'''
    always_run = True 

    def _find_closest_tag(self, ctx):
        cmd = [
            'git',
            'describe',
            '--tags',
            '--dirty',
            '--long'
        ]
    
        try:
            out = ctx.cmd_and_log(cmd, 
                                  output=Context.STDOUT,
                                  quiet=Context.BOTH)
        except Exception as e:
            raise Exception(e.stderr.strip())
   
        matches = re.match('(.+)-(\d+)-(g[a-fA-F0-9]+)(?:-(dirty))?', out)

        if not matches:
            raise ValueError("Closest tag found in git log"
                             "does not match the expected format (tag='{}')"
                             .format(out))

        tag = matches.group(1)

        self.fields['CLOSEST_TAG'] = '"{}"'.format(tag)
        self.fields['VERSION_TAG_DISTANCE'] = matches.group(2)
        self.fields['VERSION_COMMIT_HASH'] = '"{}"'.format(matches.group(3))
        self.fields['VERSION_DIRTY_FLAG'] = '1' if matches.group(4) else '0'
        
        self._parse_version_tag(ctx, tag) 

    def _find_closest_ns3_tag(self, ctx):
        cmd = [
            'git',
            'describe',
            '--tags',
            '--abbrev=0',
            '--match',
            'ns-3*',
            'HEAD'
        ]
    
        try:
            out = ctx.cmd_and_log(cmd, 
                                  output=Context.STDOUT,
                                  quiet=Context.BOTH)
        except Exception as e:
            raise Exception(e.stderr.strip())

        tag = out.strip() 

        result = self._parse_version_tag(ctx, tag)

        if not result:
            raise ValueError("Closest ns3 tag found in git log"
                             " does not match the expected format (tag='{}')"
                             .format(tag))

@TaskGen.feature('version-defines')
def generate_version_defines(self):

    #Create a substitution task to generate version-defines.h
    #from fields stored in env 
    subst_task = self.create_task('subst', self.source, self.target)

    if self.env['HAVE_NS3_REPO']:
        #if a git repo is present, run the version task first to
        #populate the appropriate fields with data from the git repo
        version_task = self.create_task('git_ns3_version_info')
        subst_task.set_run_after(version_task)

@Configure.conf
def check_git_repo(self):
    '''Determine if a git repository is present'''

    root = False
    cmd = [
            'git',
            'rev-parse',
            '--show-toplevel'
    ]

    try:
        #determine if the current directory is part of a git repository 
        self.find_program('git')
    
        out = self.cmd_and_log(cmd, output=Context.STDOUT, quiet=Context.BOTH)
    
        root = out.strip() 
    except Exception:
        root = False
    
    self.msg('Checking for local git repository', root)
    
    return bool(root) 

@Configure.conf
def check_git_repo_has_ns3_tags(self):
    '''Determine if the git repository is an ns-3 repository

       A repository is considered an ns-3 repository if it has at least one 
       tag that matches the regex ns-3*
    '''

    tag = False

    cmd = [
        'git',
        'describe',
        '--tags',
        '--abbrev=0',
        '--match',
        'ns-3.[0-9]*'
    ]

    try:
        out = self.cmd_and_log(cmd, output=Context.STDOUT, quiet=Context.BOTH)

        tag = out.strip()

    except Exception:
        tag = False

    self.msg('Checking local git repository for ns3 tags', tag)

    return bool(tag) 

def configure(ctx):

    has_git_repo = False
    has_ns3_tags = False

    if not Options.options.enable_build_version:
        return

    if ctx.check_git_repo():
        has_git_repo = True
        has_ns3_tags = ctx.check_git_repo_has_ns3_tags()

    ctx.env['HAVE_GIT_REPO'] = has_git_repo
    ctx.env['HAVE_NS3_REPO'] = has_ns3_tags

    if not has_ns3_tags:
        version_cache = ConfigSet.ConfigSet ()

        #no ns-3 repository, look for a cache file containing the version info
        ctx.start_msg('Searching for file {}'.format(CACHE_FILE))

        glob_pattern = '**/{}'.format(CACHE_FILE)
        cache_path = ctx.path.ant_glob(glob_pattern)

        if len(cache_path) > 0:
            #Found cache file
            #Load it and merge the information into the main context environment
            src_path = cache_path[0].srcpath()
            ctx.end_msg(src_path)

            version_cache.load (src_path)
        else:
            ctx.end_msg(False)

            ctx.fatal('Unable to find ns3 git repository or version.cache file '
                        'containing version information')

        ctx.env.update(version_cache)


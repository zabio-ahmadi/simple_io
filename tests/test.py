#!/usr/bin/env python3
from genericpath import isfile
import os
import tempfile
import filecmp
import time
import subprocess
import logging
import pathlib
import shutil
from colorama import Fore
import argparse

# TODO: How to include the folder creation in the test function instead of repeating it for all test functions
# TODO: pass over EVERY ASSERTION ERROR (i.e. continu test). But seems unlikely to be done easily

def test(func):
    def wrapper(self):
        print(func.__name__ + ": ", end="")
        try:
            func(self)
        except Exception as e:
            print(Fore.RED + "FAILED" + Fore.RESET)
            logging.error(f"{type(e).__name__}:{str(e)}")
            return
        print(Fore.GREEN + "SUCCESS" + Fore.RESET)

    return wrapper

class Test:


    def __init__(self, exec, root=None, delete_root=True) -> None:
        self.exec = exec
        self.delete_root = delete_root
        if root is None:
            root =  tempfile.mkdtemp(prefix='ultra-cp-')
            self.root = pathlib.Path(root)
            self.root.chmod(0o750)  # TODO: add as command line option ?
        else:
            root=pathlib.Path('root')


    def __del__(self):
        if self.delete_root:
            self.delete_folder(self.root)


    def delete_folder(self, path):
        path.chmod(0o700)
        for child in path.iterdir():
            if child.is_file() or child.is_symlink():
                child.unlink(missing_ok=True)
            else:
                self.delete_folder(child)
        path.rmdir()


    def populate_folder(self, path, no_perms=False):
        # Create folder architecture with files and links
        path.mkdir(parents=True)
        for i in range(2):
            dir = path.joinpath(f'dir{i}')
            dir.mkdir()
            for j, mode in enumerate([0o444, 0o666, 0o777]):
                file = dir.joinpath(f'file{j}')
                with open(file, 'w') as f:
                    for k in range(0,j+1):
                        f.writelines(f'{k}')
                file.chmod(mode)
            subdir = dir.joinpath('subdir')
            subdir.mkdir()
            subdir.joinpath('link0').symlink_to(pathlib.Path('..').joinpath('file0'))

        # Add a file and a folder with no read permission (these should not be tested for copies)
        if no_perms:
            dir = path.joinpath('nonreadabledir')
            dir.mkdir()
            dir.chmod(0o000)
            file = path.joinpath('nonreadablefile')
            file.touch()
            file.chmod(0o000)


    def compare_dirs(self, src, dest):
        for src_path in src.rglob('*'):
            relative_path = src_path.relative_to(src)
            dest_path = dest.joinpath(relative_path)
            if src_path.is_file() and not filecmp.cmp(src_path, dest_path, shallow=False):
                raise AssertionError(f'Content of {src_path} and {dest_path} are not identical')

    # TODO: this should be decomposed in two tests
    # tests should be able to print a validate message or not
    # here the test should not output a validate message but just make catch and print the exception
    def test_listing(self, no_perms):
        folder = self.root.joinpath('test_listing')
        self.populate_folder(folder, no_perms=no_perms)
        print('Listing fichier:')
        try:
            subprocess.run([self.exec, folder.joinpath('dir0/file0')], check=True)
        except Exception as e:
            logging.error(f"{type(e).__name__}:{str(e)}")
            logging.error("Cannot show more (or any ?) program output as the program failed with the above error")

        print('Listing dossier:')
        try:
            subprocess.run([self.exec, folder], check=True)
        except Exception as e:
            logging.error(f"{type(e).__name__}:{str(e)}")
            logging.error("Cannot show more (or any ?) program output as the program failed with the above error")

        input('Please validate output visually (not automatized yet) and press enter when done')


    @test
    def test_file_copy(self):
        folder = self.root.joinpath('test_file_copy')
        src = folder.joinpath('src')
        file = 'file0'
        self.populate_folder(src)
        infile = src.joinpath('dir0', 'file0')
        outfile = folder.joinpath('file0')
        subprocess.run([self.exec, infile, outfile], check=True)
        fileEquals = filecmp.cmp(infile, outfile, shallow=False)
        if not fileEquals:
            raise AssertionError(f'Content of {infile} and {outfile} are not identical, file copy to file does not work')


    @test
    def test_file_dir_copy(self):
        folder = self.root.joinpath('test_file_dir_copy')
        src = folder.joinpath('src')
        self.populate_folder(src)
        dest = folder.joinpath('dest')
        dest.mkdir(parents=True)
        infile = src.joinpath('dir0','file0')
        outfile = dest.joinpath('file0')
        subprocess.run([self.exec, infile, dest], check=True)
        fileEquals = filecmp.cmp(infile, outfile, shallow=False)
        if not fileEquals:
            raise AssertionError(f'Content of {infile} and {outfile} are not identical, file to dir does not work')


    @test
    def test_dir_copy(self):
        folder = self.root.joinpath('test_dir_copy')
        src = folder.joinpath('src')
        self.populate_folder(src)
        indir0 = src.joinpath('dir0')
        indir1 = src.joinpath('dir1')
        dest = folder.joinpath('dest')
        dest.mkdir(parents=True)
        subprocess.run([self.exec, indir0, indir1, dest], check=True)
        self.compare_dirs(indir0, dest.joinpath('dir0'))
        self.compare_dirs(indir1, dest.joinpath('dir1'))


    @test
    def test_dir_copy_umask(self):
        folder = self.root.joinpath('test_dir_copy_umask')
        src = folder.joinpath('src')
        self.populate_folder(src)
        dest = folder.joinpath('dest')
        dest.mkdir(parents=True)
        old_umask = os.umask(0o777)
        try:
            subprocess.run([self.exec, src, dest], check=True)
        except Exception as e:
            os.umask(old_umask)
            raise e
        os.umask(old_umask)
        self.compare_dirs(src, dest.joinpath('src'))


    @test
    def test_delete_if_not_in_source(self):
        folder = self.root.joinpath('test_delete_if_not_in_source')
        src = folder.joinpath('src')
        self.populate_folder(src)
        dest = folder.joinpath('dest')
        shutil.copytree(src, dest.joinpath('src'))
        src.joinpath('dir0', 'file1').unlink()
        subprocess.run([self.exec, src, dest], check=True)
        if not dest.joinpath('src', 'dir0', 'file1').is_file():
            raise AssertionError('Deleted file in source was also deleted in destination')

    @test
    def test_replace_time_or_size(self):
        # TODO: for the moment only test the date cause size different + date not changed is a very special case
        # TODO: there is a one second interval cause students can use the modified time with second precision -> this should be clarified in the course and possibly change to ns precision
        folder = self.root.joinpath('test_replace_time_or_size')
        src = folder.joinpath('src')
        self.populate_folder(src)
        dest = folder.joinpath('dest')
        time.sleep(1.1)  # to ensure that times are different of at least 1 second
        shutil.copytree(src, dest.joinpath('src'), copy_function=shutil.copy)
        time.sleep(1.1)  # to ensure that times are different of at least 1 second
        src.joinpath('dir0', 'file1').touch() # change the date of this file (this is the only one that should be updated by the copy)
        #record two destination current dates including the file above
        file_cpy = dest.joinpath('src','dir0', 'file1')
        file_nocpy = dest.joinpath('src','dir0', 'file2')
        time_cpy = file_cpy.stat().st_mtime
        time_nocpy = file_nocpy.stat().st_mtime
        # Execute copy only the file1 time should be updated
        subprocess.run([self.exec, src, dest], check=True)
        if file_cpy.stat().st_mtime <= time_cpy:
            raise AssertionError(f'The modified file {src.joinpath("dir0", "file1")} was not copied to {file_cpy}')
        if file_nocpy.stat().st_mtime != time_nocpy:
            raise AssertionError(f'The destination file {file_nocpy} was modified by the copy (and should not be)')


    @test
    def test_option_a(self):
        # TODO: for the moment only test the date cause size different + date not changed is a very special case
        # TODO: there is a one second interval cause students can use the modified time with second precision -> this should be clarified in the course and possibly change to ns precision
        folder = self.root.joinpath('test_option_a')
        src = folder.joinpath('src')
        self.populate_folder(src)
        dest = folder.joinpath('dest')
        time.sleep(1.1)  # to ensure that times are different of at least 1 second
        shutil.copytree(src, dest.joinpath('src'), copy_function=shutil.copy)
        # change permission of a file but not its content
        # i.e. it should not be copied (cause date older) but permission should be modified in dest
        filesrc = src.joinpath('dir0', 'file1')
        filesrc.chmod(0o400)
        filedest = dest.joinpath('src','dir0', 'file1')
        timedest = filedest.stat().st_mtime

        # Execute copy. dir0/file1 should have the mode changed (but not the modification date)
        time.sleep(1.1)  # to ensure that times are different of at least 1 second
        subprocess.run([self.exec, '-a', src, dest], check=True)
        if filedest.stat().st_mtime > timedest:
            raise AssertionError(f'The destination file {filedest} was modified by the copy (and should not be)')
        if filedest.stat().st_mode != filesrc.stat().st_mode:
            raise AssertionError(f'The destination file {filedest} mode was not changed')


    @test
    def test_option_f(self):
        folder = self.root.joinpath('test_option_f')
        src = folder.joinpath('src')
        self.populate_folder(src)
        dest = folder.joinpath('dest')
        dest.mkdir(parents=True)
        subprocess.run([self.exec, '-f', src, dest], check=True)
        destlink = dest.joinpath('src', 'dir0', 'subdir', 'link0')
        if not destlink.is_symlink():
            raise AssertionError(f'The de destination {destlink} is not a symlink')
        if not destlink.readlink().is_absolute():
            raise AssertionError(f'The de destination {destlink} is not an absolute path')


if __name__=="__main__":
    parser = argparse.ArgumentParser(description='Test an ultra-cp program according to the I/O TP of the course "system programming".')
    parser.add_argument('prog_path', help='the tested program path (must be an executable file)')
    parser.add_argument('-p', '--no-zero-mode', dest='no_perms', action='store_false', help='tests will NOT include files without any permission (only applies to concerned tests)')
    args = parser.parse_args()
    t = Test(args.prog_path)
    t.test_listing(args.no_perms)
    t.test_file_copy()
    t.test_file_dir_copy()
    t.test_dir_copy()
    t.test_dir_copy_umask()
    t.test_delete_if_not_in_source()
    t.test_replace_time_or_size()
    t.test_option_a()
    t.test_option_f()
    input('Tests are over press a key and directory will be cleaned')

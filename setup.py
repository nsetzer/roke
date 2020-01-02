#! python setup.py install
from setuptools import setup, Command
import os, sys
import platform
from distutils.sysconfig import get_python_lib
from setuptools import setup, Command
import subprocess
import shutil

entry_points = [
    "roke-qt=pyroke.gui:main",
]

def BuildArtifact(so_name):
    """ find the platform dependant build artifact """

    platname = platform.system().lower()
    so_prefix = "lib"

    if platname == "windows":
        build_path = os.path.join(os.getcwd(), "MSVC", "bin", "Release")
        so_ext = "dll"
        so_prefix = ""
    elif platname == "darwin":
        build_path = os.path.join(os.getcwd(), "release-osx", "lib")
        so_ext = "dylib"
    else:
        build_path = os.path.join(os.getcwd(), "release", "lib")
        so_ext = "so"

    libroke_name = "%s%s.%s" % (so_prefix, so_name, so_ext)

    return os.path.join(build_path, libroke_name)

libroke_path = BuildArtifact("roke")
if not os.path.exists(libroke_path):
    raise FileNotFoundError("not found: %s" % libroke_path)
libtarget = os.path.join(get_python_lib(), "pyroke")

data_files = [ (libtarget, [libroke_path]), ]

class Fbs(Command):
    """ execute the fbs module

    """
    description = __doc__
    user_options = []
    action = ""

    def initialize_options(self):
        pass

    def finalize_options(self):
        pass

    def run(self, *args):
      self._exec()

    def _exec(self):
      args = [sys.executable, "-m", "fbs", self.action]
      subprocess.Popen(args).communicate()

class FbsRun(Fbs):
    action = "run"

class FbsFreeze(Fbs):
    action = "freeze"

    def run(self, *args):
      libroke_name=os.path.split(libroke_path)[1]
      libroke_base = os.path.join("src", "main", "resources", "base")
      shutil.copy(libroke_path, libroke_base)
      self._exec()

setup(name="pyroke",
      version='1.0',
      description="qt gui for libroke",
      packages=["pyroke",],
      entry_points={"gui_scripts":entry_points},
      data_files=data_files,
      cmdclass = {'run': FbsRun,
                  'freeze' : FbsFreeze},
      )

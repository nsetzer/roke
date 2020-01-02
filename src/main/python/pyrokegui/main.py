import sys
from fbs_runtime.application_context import ApplicationContext, \
    cached_property
from pyroke.gui import MainWindow, handle_exception

class AppContext(ApplicationContext):
    def run(self):
        self.main_window.show()
        return self.app.exec_()

    @cached_property
    def main_window(self):
        sys.excepthook = handle_exception

        try:
            mw = MainWindow()
        except Exception as e:
            with open("error.log", "w") as wf:
                wf.write("%s\n" % e)

        return mw

if __name__ == '__main__':

    appctxt = AppContext()
    exit_code = appctxt.run()
    sys.exit(exit_code)
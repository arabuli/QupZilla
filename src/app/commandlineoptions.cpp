#include "commandlineoptions.h"
#include "qupzilla.h"

CommandLineOptions::CommandLineOptions(int &argc, char **argv) :
  QObject(0)
  ,m_actionString("")
  ,m_argc(argc)
  ,m_argv(argv)
  ,m_action(NoAction)
{
    parseActions();
}

void CommandLineOptions::showHelp()
{
    using namespace std;

    const char* help= " Usage: qupzilla [options] URL  \n"
                      "\n"
                      " QupZilla options:\n"
                      "    -h or -help                  print this message \n"
                      "    -a or -authors               print QupZilla authors \n"
                      "    -v or -version               print QupZilla version \n"
                      "    -p or -profile=PROFILE       start with specified profile \n"
                      "    -np or -no-plugins           start without plugins \n"
                      "\n"
                      " QupZilla is a new, fast and secure web browser\n"
                      " based on WebKit core (http://webkit.org) and\n"
                      " written in Qt Framework (http://qt.nokia.com) \n"
                      ;
    cout << help << " " << QupZilla::WWWADDRESS.toAscii().data() << endl;
}

void CommandLineOptions::parseActions()
{
    using namespace std;

    bool found = false;
    // Skip first argument (program itself)
    for (int i = 1; i < m_argc; i++) {
        QString arg(m_argv[i]);
        if (arg == "-h" || arg == "-help") {
            showHelp();
            found = true;
            break;
        }
        if (arg == "-a" || arg == "-authors") {
            cout << "QupZilla authors: " << endl;
            cout << "  nowrep <nowrep@gmail.com>" << endl;
            found = true;
            break;
        }
        if (arg == "-v" || arg == "-version") {
            cout << "QupZilla v" << QupZilla::VERSION.toAscii().data()
                 << "(build " << QupZilla::BUILDTIME.toAscii().data() << ")"
                 << endl;
            found = true;
            break;
        }

        if (arg.startsWith("-p=") || arg.startsWith("-profile=")) {
            arg.remove("-p=");
            arg.remove("-profile=");
            found = true;
            cout << "starting with profile " << arg.toAscii().data() << endl;
            m_actionString = arg;
            m_action = StartWithProfile;
        }

        if (arg.startsWith("-np") || arg.startsWith("-no-plugins")) {
            found = true;
            m_action = StartWithoutAddons;
        }
    }

    QString url(m_argv[m_argc-1]);
    if (m_argc > 1 && !url.isEmpty() && !url.startsWith("-")) {
        found = true;
        cout << "starting with url " << url.toAscii().data() << endl;
        m_actionString = url;
        m_action = OpenUrl;
    }

    if (m_argc > 1 && !found) {
        cout << "bad arguments!" << endl;
        showHelp();
    }

}
CommandLineOptions::Action CommandLineOptions::getAction()
{
    return m_action;
}

QString CommandLineOptions::getActionString()
{
    return m_actionString;
}


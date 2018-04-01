#ifndef PLUGINS
#define PLUGINS

#include <QPixmap>
#include <QString>
#include <QSize>

#define MILLA_PLUGIN_GEN_LID "org.MatrixS_Master.MILLA.Plugins.Generator"

class MillaGeneratorPlugin
{
public:
    virtual ~MillaGeneratorPlugin() {}

    virtual void setImageSize(QSize sz) = 0;
    virtual QSize getImageSize() = 0;

    virtual bool config(QString cfg) = 0;
    virtual QPixmap generate() = 0;
    virtual void update() = 0;
};

Q_DECLARE_INTERFACE(MillaGeneratorPlugin, MILLA_PLUGIN_GEN_LID)


#endif // PLUGINS


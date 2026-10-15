#pragma once
#include "gui/Presentation.h"
#include <QDialog>
class QLineEdit;
class QCheckBox;
class QLabel;
namespace gui {
class NewProcessDialog : public QDialog {
public:
    explicit NewProcessDialog(QWidget* parent = nullptr);
    api::AltaProceso input() const;
    void accept() override;
private:
    QLineEdit *pid_, *name_, *bytes_, *cpu_, *ioAfter_, *ioWait_;
    QCheckBox* io_;
    QLabel* error_;
};
}

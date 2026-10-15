#include "gui/NewProcessDialog.h"
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QRegularExpressionValidator>
#include <QVBoxLayout>
namespace gui {
NewProcessDialog::NewProcessDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Nuevo proceso");
    auto* layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel("La prioridad y la cuota las publica el motor.", this));
    auto* form = new QFormLayout;
    auto numeric = [this](const char* id, const QString& initial) {
        auto* edit = new QLineEdit(initial, this);
        edit->setObjectName(id);
        edit->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]{1,19}"), edit));
        return edit;
    };
    pid_ = numeric("inputPid", "1");
    name_ = new QLineEdit(this);
    name_->setObjectName("inputName");
    bytes_ = numeric("inputBytes", "64");
    cpu_ = numeric("inputCpu", "20");
    ioAfter_ = numeric("inputIoAfter", "8");
    ioWait_ = numeric("inputIoWait", "10");
    io_ = new QCheckBox("Habilitar una operación de E/S", this);
    io_->setObjectName("inputIo");
    form->addRow("PID", pid_);
    form->addRow("Nombre", name_);
    form->addRow("Tamaño (bytes)", bytes_);
    form->addRow("CPU total (ms simulados)", cpu_);
    form->addRow(io_);
    form->addRow("E/S después de CPU (ms)", ioAfter_);
    form->addRow("Duración de espera (ms)", ioWait_);
    ioAfter_->setEnabled(false);
    ioWait_->setEnabled(false);
    connect(io_, &QCheckBox::toggled, ioAfter_, &QWidget::setEnabled);
    connect(io_, &QCheckBox::toggled, ioWait_, &QWidget::setEnabled);
    layout->addLayout(form);
    error_ = new QLabel(this);
    error_->setObjectName("inputError");
    error_->setWordWrap(true);
    layout->addWidget(error_);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &NewProcessDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
}
api::AltaProceso NewProcessDialog::input() const {
    api::AltaProceso result;
    bool valid = false;
    result.pid = pid_->text().toInt(&valid);
    if (!valid) result.pid = -1;
    result.nombre = name_->text().trimmed().toStdString();
    auto parse = [](const QLineEdit* edit) -> std::int64_t {
        bool ok = false;
        const auto value = edit->text().toLongLong(&ok);
        return ok ? value : -1;
    };
    result.tamanioBytes = parse(bytes_);
    result.tiempoCpuTotalMs = parse(cpu_);
    result.operacionIO = {io_->isChecked(), parse(ioAfter_), parse(ioWait_)};
    return result;
}
void NewProcessDialog::accept() {
    const auto message = validateInput(input());
    error_->setText(QString::fromStdString(message));
    if (message.empty()) QDialog::accept();
}
}

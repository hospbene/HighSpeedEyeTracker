#include <QtGui>
#include <qpushbutton.h>
#include <qdialog.h>
#include <qformlayout.h>
#include <qlineedit.h>

class Dialog : public QDialog
{
public:
	QLineEdit *edit;
	Dialog()
	{
		QDialog *subDialog = new QDialog;

		QGroupBox *formGroupBox;
		QDialogButtonBox *buttonBox;
		formGroupBox = new QGroupBox(tr("Form layout"));
		buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
		
		QFormLayout *layout = new QFormLayout;
		edit = new QLineEdit();
		layout->addRow(new QLabel(tr("Name:")), edit);
		layout->addRow(buttonBox);
		connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
		connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
		formGroupBox->setLayout(layout);

		setLayout(layout);

		setWindowTitle(tr("New User"));
		
	}

	QString getOutput()
	{
		return edit->text();
	}


};
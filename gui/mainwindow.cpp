/* Copyright 2013 MultiMC Contributors
 *
 * Authors: Andrew Okin
 *          Peterix
 *          Orochimarufan <orochimarufan.x3@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QMenu>
#include <QMessageBox>
#include <QInputDialog>
#include <QApplication>
#include <QDesktopServices>
#include <QUrl>
#include <QDir>
#include <QFileInfo>

#include "osutils.h"
#include "userutils.h"
#include "pathutils.h"

#include "gui/settingsdialog.h"
#include "gui/newinstancedialog.h"
#include "gui/logindialog.h"
#include "gui/taskdialog.h"
#include "gui/browserdialog.h"
#include "gui/aboutdialog.h"

#include "kcategorizedview.h"
#include "kcategorydrawer.h"

#include "instancelist.h"
#include "appsettings.h"
#include "version.h"

#include "logintask.h"
#include <instance.h>

#include "instancemodel.h"
#include "instancedelegate.h"

// Opens the given file in the default application.
// TODO: Move this somewhere.
void openInDefaultProgram ( QString filename );

MainWindow::MainWindow ( QWidget *parent ) :
	QMainWindow ( parent ),
	ui ( new Ui::MainWindow ),
	instList ( globalSettings->get ( "InstanceDir" ).toString() )
{
	ui->setupUi ( this );
	// Create the widget
	instList.loadList();
	
	view = new KCategorizedView ( ui->centralWidget );
	drawer = new KCategoryDrawer ( view );

	view->setSelectionMode ( QAbstractItemView::SingleSelection );
	//view->setSpacing( KDialog::spacingHint() );
	view->setCategoryDrawer ( drawer );
	view->setCollapsibleBlocks ( true );
	view->setViewMode ( QListView::IconMode );
	view->setFlow ( QListView::LeftToRight );
	view->setWordWrap(true);
	view->setMouseTracking ( true );
	view->viewport()->setAttribute ( Qt::WA_Hover );
	auto delegate = new ListViewDelegate();
	view->setItemDelegate(delegate);
	view->setSpacing(10);

	model = new InstanceModel ( instList,this );
	proxymodel = new InstanceProxyModel ( this );
	proxymodel->setSortRole ( KCategorizedSortFilterProxyModel::CategorySortRole );
	proxymodel->setFilterRole ( KCategorizedSortFilterProxyModel::CategorySortRole );
	//proxymodel->setDynamicSortFilter ( true );
	proxymodel->setSourceModel ( model );
	proxymodel->sort ( 0 );

	view->setFrameShape ( QFrame::NoFrame );

	ui->horizontalLayout->addWidget ( view );
	setWindowTitle ( QString ( "MultiMC %1" ).arg ( Version::current.toString() ) );
	// TODO: Make this work with the new settings system.
//	restoreGeometry(settings->getConfig().value("MainWindowGeometry", saveGeometry()).toByteArray());
//	restoreState(settings->getConfig().value("MainWindowState", saveState()).toByteArray());
	view->setModel ( proxymodel );
	connect(view, SIGNAL(doubleClicked(const QModelIndex &)),
        this, SLOT(instanceActivated(const QModelIndex &)));

}

MainWindow::~MainWindow()
{
	delete ui;
	delete proxymodel;
	delete model;
	delete drawer;
}

void MainWindow::instanceActivated ( QModelIndex index )
{
	if(!index.isValid())
		return;
	Instance * inst = (Instance *) index.data(InstanceModel::InstancePointerRole).value<void *>();
	doLogin(inst->id());
}

void MainWindow::on_actionAddInstance_triggered()
{
	NewInstanceDialog *newInstDlg = new NewInstanceDialog ( this );
	newInstDlg->exec();
}

void MainWindow::on_actionViewInstanceFolder_triggered()
{
	openInDefaultProgram ( globalSettings->get ( "InstanceDir" ).toString() );
}

void MainWindow::on_actionRefresh_triggered()
{
	instList.loadList();
}

void MainWindow::on_actionViewCentralModsFolder_triggered()
{
	openInDefaultProgram ( globalSettings->get ( "CentralModsDir" ).toString() );
}

void MainWindow::on_actionCheckUpdate_triggered()
{

}

void MainWindow::on_actionSettings_triggered()
{
	SettingsDialog dialog ( this );
	dialog.exec();
}

void MainWindow::on_actionReportBug_triggered()
{
	//QDesktopServices::openUrl(QUrl("http://bugs.forkk.net/"));
	openWebPage ( QUrl ( "http://bugs.forkk.net/" ) );
}

void MainWindow::on_actionNews_triggered()
{
	//QDesktopServices::openUrl(QUrl("http://news.forkk.net/"));
	openWebPage ( QUrl ( "http://news.forkk.net/" ) );
}

void MainWindow::on_actionAbout_triggered()
{
	AboutDialog dialog ( this );
	dialog.exec();
}

void MainWindow::on_mainToolBar_visibilityChanged ( bool )
{
	// Don't allow hiding the main toolbar.
	// This is the only way I could find to prevent it... :/
	ui->mainToolBar->setVisible ( true );
}

void MainWindow::closeEvent ( QCloseEvent *event )
{
	// Save the window state and geometry.
	// TODO: Make this work with the new settings system.
//	settings->getConfig().setValue("MainWindowGeometry", saveGeometry());
//	settings->getConfig().setValue("MainWindowState", saveState());
	QMainWindow::closeEvent ( event );
}

void MainWindow::on_instanceView_customContextMenuRequested ( const QPoint &pos )
{
	QMenu *instContextMenu = new QMenu ( "Instance", this );

	// Add the actions from the toolbar to the context menu.
	instContextMenu->addActions ( ui->instanceToolBar->actions() );

	instContextMenu->exec ( view->mapToGlobal ( pos ) );
}


void MainWindow::on_actionLaunchInstance_triggered()
{
	QModelIndex index = view->currentIndex();
	if(index.isValid())
	{
		Instance * inst = (Instance *) index.data(InstanceModel::InstancePointerRole).value<void *>();
		doLogin(inst->id());
	}
}

void MainWindow::doLogin ( QString inst, const QString& errorMsg )
{
	LoginDialog* loginDlg = new LoginDialog ( this, errorMsg );
	if ( loginDlg->exec() )
	{
		UserInfo uInfo ( loginDlg->getUsername(), loginDlg->getPassword() );

		TaskDialog* tDialog = new TaskDialog ( this );
		LoginTask* loginTask = new LoginTask ( uInfo, inst, tDialog );
		connect ( loginTask, SIGNAL ( loginComplete ( QString, LoginResponse ) ),
		          SLOT ( onLoginComplete ( QString, LoginResponse ) ), Qt::QueuedConnection );
		connect ( loginTask, SIGNAL ( loginFailed ( QString, QString ) ),
		          SLOT ( onLoginFailed( QString, QString ) ), Qt::QueuedConnection );
		tDialog->exec ( loginTask );
	}
}

void MainWindow::onLoginComplete ( QString inst, LoginResponse response )
{
	QMessageBox::information ( this, "Login Successful",
	                           QString ( "Logged in as %1 with session ID %2. Instance: %3" ).
	                           arg ( response.username(), response.sessionID(), inst ) );
}

void MainWindow::onLoginFailed ( QString inst, const QString& errorMsg )
{
	doLogin(inst, errorMsg);
}


// Create A Desktop Shortcut
void MainWindow::on_actionMakeDesktopShortcut_triggered()
{
	QString name ( "Test" );
	name = QInputDialog::getText ( this, tr ( "MultiMC Shortcut" ), tr ( "Enter a Shortcut Name." ), QLineEdit::Normal, name );

	Util::createShortCut ( Util::getDesktopDir(), QApplication::instance()->applicationFilePath(), QStringList() << "-dl" << QDir::currentPath() << "test", name, "application-x-octet-stream" );

	QMessageBox::warning ( this, "Not useful", "A Dummy Shortcut was created. it will not do anything productive" );
}

// BrowserDialog
void MainWindow::openWebPage ( QUrl url )
{
	BrowserDialog *browser = new BrowserDialog ( this );

	browser->load ( url );
	browser->exec();
}

void openInDefaultProgram ( QString filename )
{
	QDesktopServices::openUrl ( "file:///" + QFileInfo ( filename ).absolutePath() );
}

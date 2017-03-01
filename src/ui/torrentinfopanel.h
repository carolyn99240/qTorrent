/* qTorrent - An open-source, cross-platform BitTorrent client
 * Copyright (C) 2017 Petko Georgiev
 *
 * torrentinfopanel.h
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TORRENTSTATUSBAR_H
#define TORRENTSTATUSBAR_H

#include <QTabWidget>

class QLabel;

class TorrentInfoPanel : public QTabWidget {
	Q_OBJECT

public:
	TorrentInfoPanel(QWidget* parent = nullptr);

	void refreshInfoTab();

public slots:
	void refresh();

private:
	QLabel* m_torrentName;
	QLabel* m_torrentSize;
	QLabel* m_pieceSize;
	QLabel* m_infoHash;
	QLabel* m_creationDate;
	QLabel* m_createdBy;
	QLabel* m_comment;
};

#endif // TORRENTSTATUSBAR_H

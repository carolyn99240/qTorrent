/* qTorrent - An open-source, cross-platform BitTorrent client
 * Copyright (C) 2017 Petko Georgiev
 *
 * torrentsettings.h
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

#ifndef TORRENTSETTINGS_H
#define TORRENTSETTINGS_H

#include <QString>

class TorrentSettings
{
public:
	TorrentSettings();

	/* Setters */
	void setDownloadLocation(const QString &downloadLocation);
	void setStartImmediately(bool startImmediately);
	void setSkipHashCheck(bool skipHashCheck);

	/* Getters */
	const QString &downloadLocation() const;
	bool startImmediately() const;
	bool skipHashCheck() const;

private:
	QString m_downloadLocation;
	bool m_startImmediately;
	bool m_skipHashCheck;
};

#endif // TORRENTSETTINGS_H

/*
 * Copyright (c) 2022 VG-Lab/URJC.
 *
 * Authors: Felix de las Pozas Alvarez <felix.delaspozas@urjc.es>
 *
 * This file is part of SimIL <https://github.com/vg-lab/SimIL>
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3.0 as published
 * by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#ifndef NEUROTESSMESH_LOADERTHREAD_H_
#define NEUROTESSMESH_LOADERTHREAD_H_

// Qt
#include <QThread>
#include <QDialog>

class QString;
class QProgressBar;

namespace nsol
{
  class DataSet;
}

namespace simil
{
  class SpikesPlayer;
}

namespace neurotessmesh
{
  /** \class LoaderThread
   * \brief Loads the dataset data in a thread.
   *
   */
  class LoaderThread
  : public QThread
  {
      Q_OBJECT
    public:
      enum class DataFileType
      { BlueConfig, SWC, NsolScene };

      /** \brief LoaderThread class constructor.
       * \param[in] arg1 Dataset filename.
       * \param[in] arg1 Blueconfig target.
       * \param[in] type Dataset type.
       *
       */
      explicit LoaderThread(const std::string &arg1, const std::string &arg2,
                            const DataFileType type);

      /** \brief LoaderThread class virtual destructor.
       *
       */
      virtual ~LoaderThread()
      {};

      /** \brief Returns the dataset.
       *
       */
      nsol::DataSet *getDataset() const
      { return m_dataset; }

      /** \brief Returns the spikes player.
       *
       */
      simil::SpikesPlayer *getPlayer() const
      { return m_player; }

      virtual void run();

      /** \brief Returns the error description or empty if none.
       *
       */
      QString errors() const
      { return m_errors; }

    signals:
      void progress(const QString &text, const unsigned int value);

    private:
      const std::string  m_fileName; /** name of file to load. */
      const std::string  m_target;   /** blueconfig target.    */
      const DataFileType m_type;     /** type of file to load. */

      nsol::DataSet       *m_dataset; /** nsol dataset with data.      */
      simil::SpikesPlayer *m_player;  /** spikes data or null if none. */

      QString m_errors;
  };

  class LoadingDialog
    : public QDialog
  {
      Q_OBJECT
    public:
      /** \brief LoadingDialog class constructor.
       * \param[in] p Raw pointer of the widget parent of this one.
       * \param[in] f QDialog flags.
       *
       */
      explicit LoadingDialog( QWidget* p = nullptr );

      /** \brief LoadingDialog class virtual destructor.
       *
       */
      virtual ~LoadingDialog( )
      { };

    public slots:

      /** \brief Updates the dialog with the message and progress value
       * \param[in] message Progress message.
       * \param[in] value Progress value in [0,100].
       *
       */
      void progress( const QString& message , const unsigned int value );

      /** \brief Closes and deletes the dialog.
       *
       */
      void closeDialog( );

    private:
      QProgressBar* m_progress; /** progress bar. */
  };

}
#endif /* NEUROTESSMESH_LOADERTHREAD_H_ */

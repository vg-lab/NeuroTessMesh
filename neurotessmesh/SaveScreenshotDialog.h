/*
 * Copyright (c) 2024 VG-Lab/URJC.
 *
 * Authors: Felix de las Pozas Alvarez <felix.delaspozas@urjc.es>
 *
 * This file is part of NeuroTessMesh <https://github.com/vg-lab/neurotessmesh>
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

#ifndef SAVE_SCREENSHOT_DIALOG_H_
#define SAVE_SCREENSHOT_DIALOG_H_

// NeuroTessMesh
#include "ui_SaveScreenshotDialog.h"

// Qt
#include <QDialog>
#include <QSize>

class SaveScreenshotDialog
    : public QDialog,
      private Ui::SaveScreenshotDialog
{
  Q_OBJECT

public:
  /** \brief SaveScreenshotDialog class constructor.
   * \param[in] width image width.
   * \param[in] height image height.
   * \param[in] image image to show.
   * \param[in] parent pointer of the QWidget parent of this one.
   */
  SaveScreenshotDialog(const int width,
                       const int height,
                       const QImage &image,
                       QWidget *parent = nullptr);

  /** \brief SaveScreenshotDialog class destructor.
   */
  virtual ~SaveScreenshotDialog(){};

  /** \brief Return the ratio of the image.
   */
  double getRatio() const;

  /** \brief Return the initial height of the image.
   */
  int getInitialHeight() const;

  /** \brief Return the initial width of the image.
   */
  int getInitialWidth() const;

  /** \brief Return the height of the image.
   */
  int getHeight() const;

  /** \brief Return the width of the image.
   */
  int getWidth() const;

  /** \brief Return the initial size of the image.
   */
  QSize getInitialSize() const;

  /** \brief Return the size of the image.
   */
  QSize getSize() const;

  /** \brief Return magnification based on expected size.
   */
  int getMagnifcation() const;

private slots:
  void onHeightChanged(int value);
  void onWidthChanged(int value);

private:
  const double m_ratio;      /** height/width ratio. */
  const QSize m_initialSize; /** initial image size. */
  QSize m_size;              /** current image size. */
};

#endif // SAVE_SCREENSHOT_DIALOG_H_

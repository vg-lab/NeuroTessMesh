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

// NeuroTessMesh
#include "SaveScreenshotDialog.h"

// Qt
#include <QDialog>
#include <QSpinBox>
#include <QDebug>

//-----------------------------------------------------------------------------
SaveScreenshotDialog::SaveScreenshotDialog(const int width,
                                           const int height,
                                           const QImage &image,
                                           QWidget *parent)
    : QDialog{parent}, m_ratio{double(width) / height}, m_initialSize{width, height}, m_size{width, height}
{
  setupUi(this);
  setWindowIcon(QIcon(":/icons/rsc/screenshot.svg"));

  m_width_spinBox->setValue(width);
  m_height_spinBox->setValue(height);

  auto thumb = image.scaled(m_image_label->maximumSize(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
  m_image_label->setFixedSize(thumb.size());
  m_image_label->setPixmap(QPixmap::fromImage(thumb));

  // minimal resolution is 100, maintain ratio but do not allow values under that
  if (width <= height)
  {
    m_width_spinBox->setMinimum(100);
    m_height_spinBox->setMinimum(100 / m_ratio);
  }
  else
  {
    m_width_spinBox->setMinimum(100 * m_ratio);
    m_height_spinBox->setMinimum(100);
  }

  connect(m_width_spinBox, SIGNAL(valueChanged(int)),
          this, SLOT(onWidthChanged(int)));
  connect(m_height_spinBox, SIGNAL(valueChanged(int)),
          this, SLOT(onHeightChanged(int)));
}

//-----------------------------------------------------------------------------
int SaveScreenshotDialog::getMagnifcation() const
{
  double re = m_size.width() / m_initialSize.width() + 0.5;
  return (re < 1) ? 1 : re;
}

//-----------------------------------------------------------------------------
void SaveScreenshotDialog::onHeightChanged(int value)
{
  m_size.setHeight(value);
  m_size.setWidth(value * m_ratio);

  m_width_spinBox->blockSignals(true);
  m_width_spinBox->setValue(m_size.width());
  m_width_spinBox->blockSignals(false);
}

//-----------------------------------------------------------------------------
void SaveScreenshotDialog::onWidthChanged(int value)
{
  m_size.setWidth(value);
  m_size.setHeight(value / m_ratio);
  m_height_spinBox->blockSignals(true);
  m_height_spinBox->setValue(m_size.height());
  m_height_spinBox->blockSignals(false);
}

//-----------------------------------------------------------------------------
double SaveScreenshotDialog::getRatio() const
{
  return m_ratio;
}

//-----------------------------------------------------------------------------
int SaveScreenshotDialog::getInitialHeight() const
{
  return m_initialSize.height();
}

//-----------------------------------------------------------------------------
int SaveScreenshotDialog::getInitialWidth() const
{
  return m_initialSize.width();
}

//-----------------------------------------------------------------------------
int SaveScreenshotDialog::getHeight() const
{
  return m_size.height();
}

//-----------------------------------------------------------------------------
int SaveScreenshotDialog::getWidth() const
{
  return m_size.width();
}

//-----------------------------------------------------------------------------
QSize SaveScreenshotDialog::getInitialSize() const
{
  return m_initialSize;
}

//-----------------------------------------------------------------------------
QSize SaveScreenshotDialog::getSize() const
{
  return m_size;
}

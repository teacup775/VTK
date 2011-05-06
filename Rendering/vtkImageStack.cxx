/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageStack.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkImageStack.h"
#include "vtkImageSliceCollection.h"
#include "vtkImageProperty.h"
#include "vtkImageMapper3D.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkImageStack);

//----------------------------------------------------------------------------
vtkImageStack::vtkImageStack()
{
  this->Images = vtkImageSliceCollection::New();
  this->ImageMatrices = NULL;
  this->ActiveLayer = 0;
}

//----------------------------------------------------------------------------
vtkImageStack::~vtkImageStack()
{
  if (this->Images)
    {
    vtkCollectionSimpleIterator pit;
    this->Images->InitTraversal(pit);
    vtkImageSlice *image = 0;
    while ( (image = this->Images->GetNextImage(pit)) != 0)
      {
      image->RemoveConsumer(this);
      }

    this->Images->Delete();
    }

  if (this->ImageMatrices)
    {
    this->ImageMatrices->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkImageStack::AddImage(vtkImageSlice *prop)
{
  if (!this->Images->IsItemPresent(prop) &&
      !vtkImageStack::SafeDownCast(prop))
    {
    this->Images->AddItem(prop);
    prop->AddConsumer(this);
    }
}

//----------------------------------------------------------------------------
void vtkImageStack::RemoveImage(vtkImageSlice *prop)
{
  if (this->Images->IsItemPresent(prop))
    {
    prop->RemoveConsumer(this);
    this->Images->RemoveItem(prop);
    }
}

//----------------------------------------------------------------------------
void vtkImageStack::GetImages(vtkPropCollection *vc)
{
  vtkCollectionSimpleIterator pit;
  this->Images->InitTraversal(pit);
  vtkImageSlice *image = 0;
  while ( (image = this->Images->GetNextImage(pit)) != 0)
    {
    image->GetImages(vc);
    }
}

//----------------------------------------------------------------------------
void vtkImageStack::ShallowCopy(vtkProp *prop)
{
  vtkImageStack *v = vtkImageStack::SafeDownCast(prop);

  if (v != NULL)
    {
    this->Images->RemoveAllItems();
    vtkCollectionSimpleIterator pit;
    v->Images->InitTraversal(pit);
    vtkImageSlice *image = 0;
    while ( (image = v->Images->GetNextImage(pit)) != 0)
      {
      this->Images->AddItem(image);
      }
    this->SetActiveLayer(v->GetActiveLayer());
    }

  // Now do prop superclass (NOT vtkImageSlice)
  this->vtkProp3D::ShallowCopy(prop);
}

//----------------------------------------------------------------------------
void vtkImageStack::SetProperty(vtkImageProperty *)
{
  // do nothing
}

//----------------------------------------------------------------------------
vtkImageProperty *vtkImageStack::GetProperty()
{
  // Get the property with the active layer number
  vtkCollectionSimpleIterator pit;
  this->Images->InitTraversal(pit);
  vtkImageSlice *image = 0;
  while ( (image = this->Images->GetNextImage(pit)) != 0)
    {
    vtkImageProperty *p = image->GetProperty();
    if (p->GetLayerNumber() == this->ActiveLayer)
      {
      return p;
      }
    }

  // Return a dummy property, can't return NULL.
  if (this->Property == 0)
    {
    this->Property = vtkImageProperty::New();
    this->Property->Register(this);
    this->Property->Delete();
    }
  return this->Property;
}

//----------------------------------------------------------------------------
void vtkImageStack::SetMapper(vtkImageMapper3D *)
{
  // do nothing
}

//----------------------------------------------------------------------------
vtkImageMapper3D *vtkImageStack::GetMapper()
{
  // Get the mapper with the active layer number
  vtkCollectionSimpleIterator pit;
  this->Images->InitTraversal(pit);
  vtkImageSlice *image = 0;
  while ( (image = this->Images->GetNextImage(pit)) != 0)
    {
    vtkImageProperty *p = image->GetProperty();
    if (p->GetLayerNumber() == this->ActiveLayer)
      {
      return image->GetMapper();
      }
    }

  return NULL;
}

//----------------------------------------------------------------------------
double *vtkImageStack::GetBounds()
{
  double bounds[6];
  bool nobounds = true;

  bounds[0] = VTK_DOUBLE_MAX;
  bounds[1] = VTK_DOUBLE_MIN;
  bounds[2] = VTK_DOUBLE_MAX;
  bounds[3] = VTK_DOUBLE_MIN;
  bounds[4] = VTK_DOUBLE_MAX;
  bounds[5] = VTK_DOUBLE_MIN;

  vtkCollectionSimpleIterator pit;
  this->Images->InitTraversal(pit);
  vtkImageSlice *image = 0;
  while ( (image = this->Images->GetNextImage(pit)) != 0)
    {
    double *b = image->GetBounds();
    if (b)
      {
      nobounds = false;
      bounds[0] = (bounds[0] < b[0] ? bounds[0] : b[0]);
      bounds[1] = (bounds[1] > b[1] ? bounds[1] : b[1]);
      bounds[2] = (bounds[2] < b[2] ? bounds[2] : b[2]);
      bounds[3] = (bounds[3] > b[3] ? bounds[3] : b[3]);
      bounds[4] = (bounds[4] < b[4] ? bounds[4] : b[4]);
      bounds[5] = (bounds[5] > b[5] ? bounds[5] : b[5]);
      }
    }

  if (nobounds)
    {
    return 0;
    }

  this->Bounds[0] = bounds[0];
  this->Bounds[1] = bounds[1];
  this->Bounds[2] = bounds[2];
  this->Bounds[3] = bounds[3];
  this->Bounds[4] = bounds[4];
  this->Bounds[5] = bounds[5];

  return this->Bounds;
}

//----------------------------------------------------------------------------
// Does this prop have some translucent polygonal geometry?
int vtkImageStack::HasTranslucentPolygonalGeometry()
{
  vtkCollectionSimpleIterator pit;
  this->Images->InitTraversal(pit);
  vtkImageSlice *image = 0;
  while ( (image = this->Images->GetNextImage(pit)) != 0)
    {
    if (image->HasTranslucentPolygonalGeometry())
      {
      return 1;
      }
    }

  return 0;
}

//----------------------------------------------------------------------------
// Assembly-like behavior
void vtkImageStack::PokeMatrices(vtkMatrix4x4 *matrix)
{
  if (this->ImageMatrices == NULL)
    {
    this->ImageMatrices = vtkCollection::New();
    }

  if (matrix)
    {
    vtkCollectionSimpleIterator pit;
    this->Images->InitTraversal(pit);
    vtkImageSlice *image = 0;
    while ( (image = this->Images->GetNextImage(pit)) != 0)
      {
      vtkMatrix4x4 *propMatrix = vtkMatrix4x4::New();
      propMatrix->Multiply4x4(image->GetMatrix(), matrix, propMatrix);
      image->PokeMatrix(propMatrix);
      this->ImageMatrices->AddItem(propMatrix);
      propMatrix->Delete();
      }
    }
  else
    {
    vtkCollectionSimpleIterator pit;
    this->Images->InitTraversal(pit);
    vtkImageSlice *image = 0;
    while ( (image = this->Images->GetNextImage(pit)) != 0)
      {
      image->PokeMatrix(NULL);
      }
    this->ImageMatrices->RemoveAllItems();
    }
}

//----------------------------------------------------------------------------
int vtkImageStack::RenderOpaqueGeometry(vtkViewport* viewport)
{
  vtkDebugMacro(<< "vtkImageStack::RenderOpaqueGeometry");

  // Opaque render is always called first, so sort here
  this->Images->Sort();

  if (!this->IsIdentity)
    {
    this->PokeMatrices(this->GetMatrix());
    }

  int rendered = 0;
  vtkImageSlice *image = 0;
  vtkCollectionSimpleIterator pit;

  if (this->Images->GetNumberOfItems() == 1)
    {
    // no multi-pass if only one image
    this->Images->InitTraversal(pit);
    image = this->Images->GetNextImage(pit);
    return image->RenderOpaqueGeometry(viewport);
    }

  for (int pass = 0; pass < 3; pass++)
    {
    this->Images->InitTraversal(pit);
    while ( (image = this->Images->GetNextImage(pit)) != 0)
      {
      image->SetStackedImagePass(pass);
      rendered |= image->RenderOpaqueGeometry(viewport);
      image->SetStackedImagePass(-1);
      }
    }

  if (!this->IsIdentity)
    {
    this->PokeMatrices(NULL);
    }

  return rendered;
}

//----------------------------------------------------------------------------
int vtkImageStack::RenderTranslucentPolygonalGeometry(vtkViewport* viewport)
{
  vtkDebugMacro(<< "vtkImageStack::RenderTranslucentPolygonalGeometry");

  if (!this->IsIdentity)
    {
    this->PokeMatrices(this->GetMatrix());
    }

  int rendered = 0;
  vtkImageSlice *image = 0;
  vtkCollectionSimpleIterator pit;

  if (this->Images->GetNumberOfItems() == 1)
    {
    // no multi-pass if only one image
    this->Images->InitTraversal(pit);
    image = this->Images->GetNextImage(pit);
    return image->RenderTranslucentPolygonalGeometry(viewport);
    }

  for (int pass = 1; pass < 3; pass++)
    {
    this->Images->InitTraversal(pit);
    while ( (image = this->Images->GetNextImage(pit)) != 0)
      {
      image->SetStackedImagePass(pass);
      rendered |= image->RenderTranslucentPolygonalGeometry(viewport);
      image->SetStackedImagePass(-1);
      }
    }

  if (!this->IsIdentity)
    {
    this->PokeMatrices(NULL);
    }

  return rendered;
}

//----------------------------------------------------------------------------
int vtkImageStack::RenderOverlay(vtkViewport* viewport)
{
  vtkDebugMacro(<< "vtkImageStack::RenderOverlay");

  if (!this->IsIdentity)
    {
    this->PokeMatrices(this->GetMatrix());
    }

  int rendered = 0;
  vtkImageSlice *image = 0;
  vtkCollectionSimpleIterator pit;

  if (this->Images->GetNumberOfItems() == 1)
    {
    // no multi-pass if only one image
    this->Images->InitTraversal(pit);
    image = this->Images->GetNextImage(pit);
    return image->RenderOverlay(viewport);
    }

  for (int pass = 1; pass < 3; pass++)
    {
    this->Images->InitTraversal(pit);
    while ( (image = this->Images->GetNextImage(pit)) != 0)
      {
      image->SetStackedImagePass(pass);
      rendered |= image->RenderOverlay(viewport);
      image->SetStackedImagePass(-1);
      }
    }

  if (!this->IsIdentity)
    {
    this->PokeMatrices(NULL);
    }

  return rendered;
}

//----------------------------------------------------------------------------
void vtkImageStack::ReleaseGraphicsResources(vtkWindow *win)
{
  vtkCollectionSimpleIterator pit;
  this->Images->InitTraversal(pit);
  vtkImageSlice *image = 0;
  while ( (image = this->Images->GetNextImage(pit)) != 0)
    {
    image->ReleaseGraphicsResources(win);
    }
}

//----------------------------------------------------------------------------
unsigned long int vtkImageStack::GetMTime()
{
  unsigned long mTime = this->Superclass::GetMTime();
  unsigned long t;

  // Get the max mtime of all the images
  vtkCollectionSimpleIterator pit;
  this->Images->InitTraversal(pit);
  vtkImageSlice *image = 0;
  while ( (image = this->Images->GetNextImage(pit)) != 0)
    {
    t = image->GetMTime();
    mTime = (t < mTime ? mTime : t);
    }

  return mTime;
}

//----------------------------------------------------------------------------
unsigned long int vtkImageStack::GetRedrawMTime()
{
  // Just call GetMTime on ourselves, not GetRedrawMTime
  unsigned long mTime = this->Superclass::GetMTime();
  unsigned long t;

  // Get the max mtime of all the images
  vtkCollectionSimpleIterator pit;
  this->Images->InitTraversal(pit);
  vtkImageSlice *image = 0;
  while ( (image = this->Images->GetNextImage(pit)) != 0)
    {
    t = image->GetRedrawMTime();
    mTime = (t < mTime ? mTime : t);
    }

  return mTime;
}

//----------------------------------------------------------------------------
void vtkImageStack::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "ActiveLayer: " << this->ActiveLayer << "\n";
}
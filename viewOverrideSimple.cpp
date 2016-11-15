//-
// Copyright 2015 Autodesk, Inc.  All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license agreement
// provided at the time of installation or download, or which otherwise
// accompanies this software in either electronic or hard copy form.
//+
#include "viewOverrideSimple.h"
#include <maya/M3dView.h>
#include <maya/MRenderTargetManager.h>
#include <maya/MRenderTarget.h>
#include <maya/MDrawContext.h>
#include <maya/MImage.h>
#include <maya/MGlobal.h>
// For override creation we return a UI name so that it shows up in as a
// renderer in the 3d viewport menus.


viewOverrideSimple::viewOverrideSimple( const MString & name )
: MRenderOverride( name )
, mUIName("Simple VP2 Override")
, mCurrentOperation(-1)
{
	mOperations[0] = mOperations[1] = mOperations[2] = mOperations[3] = NULL;
	mOperationNames[0] = "viewOverrideSimple_Scene";
	mOperationNames[1] = "viewOverrideSimple_Quad";
	mOperationNames[2] = "viewOverrideSimple_HUD";
	mOperationNames[3] = "viewOverrideSimple_Present";
	
	//MSAA sample count, we don't want MSAA here
	unsigned int sampleCount =1; 

	MHWRender::MRasterFormat colorFormat = MHWRender::kR32G32B32A32_FLOAT;

	//Create color buffer first
	mTargetOverrideNames[0] = MString("_viewRender_SSAA_color");
	mTargetDescriptions	[0] =
		new MHWRender::MRenderTargetDescription(mTargetOverrideNames[0], 256, 256, sampleCount, colorFormat, 1, false);
	mTargets			[0] = NULL;
	
	//We also need to created a depth buffer
	mTargetOverrideNames[1] = MString("_viewRender_SSAA_depth");
	mTargetDescriptions	[1] =
		new MHWRender::MRenderTargetDescription(mTargetOverrideNames[1], 256, 256, sampleCount, MHWRender::kD24S8, 1, false);
	mTargets			[1] = NULL;

}

// On destruction all operations are deleted.
//
viewOverrideSimple::~viewOverrideSimple()
{
	for (unsigned int i=0; i<TOTAL_RENDER_OPERATIONS; i++)
	{
		if (mOperations[i])
		{
			delete mOperations[i];
			mOperations[i] = NULL;
		}
	}

	for(int i = 0; i < 2; ++i)
	{
		if(mTargetDescriptions[i])
		{
			delete mTargetDescriptions[i];
			mTargetDescriptions[i] = NULL;
		}
		MHWRender::MRenderer* theRenderer = MHWRender::MRenderer::theRenderer();
		if (theRenderer)
		{
			auto targetManager = theRenderer->getRenderTargetManager();
			if(targetManager)
			{
				targetManager->releaseRenderTarget(mTargets[i]);
				mTargets[i] = NULL;
			}
		}
	}
}
	
// I've only written a core profile shader, so it only supports core profile now.
//
MHWRender::DrawAPI viewOverrideSimple::supportedDrawAPIs() const
{
	return MHWRender::kOpenGLCoreProfile;
}

// Basic iterator methods which returns a list of operations in order
// The operations are not executed at this time only queued for execution
//
// - startOperationIterator() : to start iterating
// - renderOperation() : will be called to return the current operation
// - nextRenderOperation() : when this returns false we've returned all operations
//
bool viewOverrideSimple::startOperationIterator()
{
	mCurrentOperation = 0;
	return true;
}

MHWRender::MRenderOperation*
viewOverrideSimple::renderOperation()
{
	if (mCurrentOperation >= 0 && mCurrentOperation < TOTAL_RENDER_OPERATIONS)
	{
		if (mOperations[mCurrentOperation])
		{
			return mOperations[mCurrentOperation];
		}
	}
	return NULL;
}

bool 
viewOverrideSimple::nextRenderOperation()
{
	mCurrentOperation++;
	if (mCurrentOperation < TOTAL_RENDER_OPERATIONS)
	{
		return true;
	}
	return false;
}

//
// On setup we make sure that we have created the appropriate operations
// These will be returned via the iteration code above.
//
// The only thing that is required here is to create:
//
//	- Two render targets
//	- One scene render operation to draw the scene.
//  - One quad render operation to down sample the scene.
//	- One "stock" HUD render operation to draw the HUD over the scene
//	- One "stock" presentation operation to be able to see the results in the viewport
//
MStatus viewOverrideSimple::setup( const MString & destination )
{
	if (!mOperations[0])
	{
		MHWRender::MRenderer *theRenderer = MHWRender::MRenderer::theRenderer();
		if (!theRenderer)
			return MStatus::kFailure;

		auto renderTargetManager = theRenderer->getRenderTargetManager();

		unsigned int targetWidth = 256;
		unsigned int targetHeight = 256;
		// Get current render target's output size
		theRenderer->outputTargetSize( targetWidth, targetHeight );

		// Double size the render target.
		targetHeight *= 2;
		targetWidth *= 2;

		for(int i = 0; i < 2; ++i)
		{
			mTargetDescriptions[i]->setWidth(targetWidth );
			mTargetDescriptions[i]->setHeight(targetHeight );

			if(!mTargets[i])
			{
				mTargets[i] = renderTargetManager->acquireRenderTarget(*mTargetDescriptions[i]);
			}
			else
			{
				mTargets[i]->updateDescription(*mTargetDescriptions[i]);
			}
		}

		mOperations[0] = (MHWRender::MRenderOperation *) new simpleViewRenderSceneRender( mOperationNames[1] );
		mOperations[1] = (MHWRender::MRenderOperation *) new simpleViewRenderQuadRender( mOperationNames[2] );
		mOperations[2] = (MHWRender::MRenderOperation *) new simpleViewRenderHudRender();
		mOperations[3] = (MHWRender::MRenderOperation *) new simpleViewRenderPresentRender( mOperationNames[3] );
	}
	if (!mOperations[0] ||
		!mOperations[1] || 
		!mOperations[2] ||
		!mOperations[3])
	{
		return MStatus::kFailure;
	}
	else
	{
		//Set custom render targets
		((simpleViewRenderSceneRender*)mOperations[0])->setRenderTargets(mTargets, 2);
		((simpleViewRenderSceneRender*)mOperations[0])->setQuadRender((simpleViewRenderQuadRender*)mOperations[1]);
	}

	return MStatus::kSuccess;
}

// On cleanup we will clean the targets for scene and quad operations
//
MStatus viewOverrideSimple::cleanup()
{
	mCurrentOperation = -1;
	auto quadOp = (simpleViewRenderQuadRender *)mOperations[1];
	if (quadOp)
	{
		quadOp->setColorTarget(NULL);
		quadOp->setDepthTarget(NULL);
		quadOp->updateTargets();
	}

	auto *sceneOp = (simpleViewRenderSceneRender *)mOperations[0];
	if (sceneOp)
	{
		sceneOp->setRenderTargets( NULL, 0 );
	}

	return MStatus::kSuccess;
}


simpleViewRenderSceneRender::simpleViewRenderSceneRender(const MString &name)
: MSceneRender( name ),  
  mTargets(NULL),
  numTargets(0),
  currentLoop(0),
  mSimpleQuadRender(NULL)
{
	 
}

void simpleViewRenderSceneRender::setRenderTargets(MHWRender::MRenderTarget **targets, unsigned int targetCount)
{
	mTargets = targets;
	numTargets = targetCount;
}

//Override render targets
MHWRender::MRenderTarget* const*  simpleViewRenderSceneRender::targetOverrideList(unsigned int &listSize)
{
	if(mTargets)
	{
 		listSize = numTargets;
		return mTargets;
	}
	listSize = 0;
	return NULL;
}


// Clear operation
MHWRender::MClearOperation &
simpleViewRenderSceneRender::clearOperation()
{
	MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();
	bool gradient = renderer->useGradient();
	MColor color1 = renderer->clearColor();
	MColor color2 = renderer->clearColor2();

	float c1[4] = { color1[0], color1[1], color1[2], 1.0f };
	float c2[4] = { color2[0], color2[1], color2[2], 1.0f };

	mClearOperation.setClearColor( c1 );
	mClearOperation.setClearColor2( c2 );
	mClearOperation.setClearGradient( gradient);
    return mClearOperation;
}

// During the post scene rendering, set current output targets as input for our quad shader.
void simpleViewRenderSceneRender::postSceneRender(const MHWRender::MDrawContext & context)
{
	if(mSimpleQuadRender)
	{				
		mSimpleQuadRender->setColorTarget(mTargets[0]);
		mSimpleQuadRender->setDepthTarget(mTargets[1]);
		mSimpleQuadRender->updateTargets();
	}
}

// Get a bicubic shader from shader effect file.
// For OpenGL core profile, it is located inside Maya/bin/OGSFX
simpleViewRenderQuadRender::simpleViewRenderQuadRender(const MString &name)
	: MQuadRender( name )
	, mShaderInstance(NULL)
	, mColorTargetChanged(false)
	, mDepthTargetChanged(false)
	, fSamplerState(NULL)
	
{
	mDepthTarget.target = NULL;
	mColorTarget.target = NULL;
	if (!mShaderInstance)
	{
		MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();
		const MHWRender::MShaderManager* shaderMgr = renderer ? renderer->getShaderManager() : NULL;
		if (shaderMgr)
		{
			//Create our bicubic shader
			mShaderInstance = shaderMgr->getEffectsFileShader( "mayaBlitColorDepthBicubic", "" );
		}		
	}
}

MHWRender::MClearOperation& simpleViewRenderQuadRender::clearOperation()
{
	
	mClearOperation.setClearGradient( false );
	mClearOperation.setMask( (unsigned int) MHWRender::MClearOperation::kClearNone );
	
	return mClearOperation;
}

// Return our bicubic shader
const MHWRender::MShaderInstance * simpleViewRenderQuadRender::shader()
{
	return mShaderInstance;
}

//Update render targets
void simpleViewRenderQuadRender::updateTargets()
{
	if(mShaderInstance)
	{
		if (!fSamplerState)
		{
			MHWRender::MSamplerStateDesc desc;
			// It should be linear when using bicubic
			desc.filter = MHWRender::MSamplerState::kMinMagMipLinear;
			
			fSamplerState = MHWRender::MStateManager::acquireSamplerState(desc);
		}
		if (fSamplerState)
		{
			mShaderInstance->setParameter("gColorSampler", *fSamplerState);
		}
		if (mColorTargetChanged)
		{
			mShaderInstance->setParameter("gColorTex", mColorTarget);
			mColorTargetChanged = false;
		}
		if (mDepthTargetChanged)
		{
			mShaderInstance->setParameter("gDepthTex", mDepthTarget);
			mDepthTargetChanged = false;
		}
	}
}
simpleViewRenderQuadRender::~simpleViewRenderQuadRender()
{
	MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();
	if (!renderer)
		return;

	// Release any shader used
	if (mShaderInstance)
	{
		const MHWRender::MShaderManager* shaderMgr = renderer->getShaderManager();
		if (shaderMgr)
		{
			shaderMgr->releaseShader(mShaderInstance);
		}
		mShaderInstance = NULL;
	}

	if (fSamplerState)
	{
		MHWRender::MStateManager::releaseSamplerState(fSamplerState);
		fSamplerState = NULL;
	}

	mColorTarget.target = NULL;
	mDepthTarget.target = NULL;
};

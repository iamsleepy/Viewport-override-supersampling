#ifndef viewOverrideSimple_h_
#define viewOverrideSimple_h_
//-
// Copyright 2016 Autodesk, Inc.  All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license agreement
// provided at the time of installation or download, or which otherwise
// accompanies this software in either electronic or hard copy form.
//+
#include <maya/MString.h>
#include <maya/MViewport2Renderer.h>
#include <maya/MPxCommand.h>
#include <maya/MShaderManager.h>

#define TOTAL_RENDER_OPERATIONS 4
//
// Simple override class derived from MRenderOverride
//
class viewOverrideSimple : public MHWRender::MRenderOverride
{
public:
	viewOverrideSimple( const MString & name );
	virtual ~viewOverrideSimple();
	virtual MHWRender::DrawAPI supportedDrawAPIs() const;

	// Basic setup and cleanup
	virtual MStatus setup( const MString & destination );
	virtual MStatus cleanup();

	// Operation iteration methods
	virtual bool startOperationIterator();
	virtual MHWRender::MRenderOperation * renderOperation();
	virtual bool nextRenderOperation();

	// UI name
	virtual MString uiName() const
	{
		return mUIName;
	}
	
protected:
	// UI name 
	MString mUIName;

	// Operations and operation names
	MHWRender::MRenderOperation* mOperations[TOTAL_RENDER_OPERATIONS];
	MString mOperationNames[TOTAL_RENDER_OPERATIONS];

	// Temporary of operation iteration
	int mCurrentOperation;

	// Supersampling render targets
	MString mTargetOverrideNames[2];
	MHWRender::MRenderTargetDescription* mTargetDescriptions[2];
	MHWRender::MRenderTarget* mTargets[2];

};
class simpleViewRenderQuadRender : public MHWRender::MQuadRender
{
public:
	simpleViewRenderQuadRender(const MString &name);
	~simpleViewRenderQuadRender();

	virtual const MHWRender::MShaderInstance * shader();
	virtual MHWRender::MClearOperation& clearOperation();
	void updateTargets();	
	inline void setColorTarget(MHWRender::MRenderTarget *target)
	{
		mColorTarget.target = target;
		mColorTargetChanged = true;
	}

	inline void setDepthTarget(MHWRender::MRenderTarget* target)
	{
		mDepthTarget.target = target;
		mDepthTargetChanged = true;
	}

protected:
	// Shader to use for the quad render
	MHWRender::MShaderInstance *mShaderInstance;

	// Render targets used for the quad render. Not owned by operation.
	MHWRender::MRenderTargetAssignment mColorTarget;
	MHWRender::MRenderTargetAssignment mDepthTarget;
	bool mColorTargetChanged;
	bool mDepthTargetChanged;
	const MHWRender::MSamplerState* fSamplerState;
};
//
// Simple scene operation override to allow for clear color
// tracking.
//
class simpleViewRenderSceneRender : public MHWRender::MSceneRender
{
public:
    simpleViewRenderSceneRender(const MString &name);
	virtual ~simpleViewRenderSceneRender(){	mTargets = NULL; mSimpleQuadRender = NULL;}
    virtual MHWRender::MClearOperation & clearOperation();
	void setRenderTargets(MHWRender::MRenderTarget **targets, unsigned int targetCount);
	virtual MHWRender::MRenderTarget* const* targetOverrideList(unsigned int &listSize);
	virtual void postSceneRender(const MHWRender::MDrawContext & context);
	inline void setQuadRender(simpleViewRenderQuadRender* quadRender){mSimpleQuadRender = quadRender;}

protected:
	MHWRender::MRenderTarget **mTargets;
	unsigned int numTargets;
	simpleViewRenderQuadRender* mSimpleQuadRender;

// Scene debug
	int currentLoop;	
};

class simpleViewRenderPresentRender : public MHWRender::MPresentTarget
{
public:
	simpleViewRenderPresentRender(const MString &name) : MPresentTarget(name){};
	virtual ~simpleViewRenderPresentRender() { };
};

class simpleViewRenderHudRender : public MHWRender::MHUDRender
{
public:
	simpleViewRenderHudRender():MHUDRender(){};
	virtual ~simpleViewRenderHudRender(){};
};

#endif

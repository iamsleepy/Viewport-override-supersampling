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

// Use depth to color technique
#define USE_DEPTH_TO_COLOR false

#define TOTAL_RENDER_OPERATIONS 6
//
// Simple override class derived from MRenderOverride
//
class ViewOverrideSimple : public MHWRender::MRenderOverride
{
public:
	ViewOverrideSimple( const MString & name );
	virtual ~ViewOverrideSimple();
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
class SimpleViewRenderQuadRender : public MHWRender::MQuadRender
{
public:
	SimpleViewRenderQuadRender(const MString& name);
	~SimpleViewRenderQuadRender();

	virtual const MHWRender::MShaderInstance * shader();
	virtual MHWRender::MClearOperation& clearOperation();
	void updateTargets();

	void setClippingPlane(const double& near, const double& far) { mNear = near; mFar = far; }

	void setColorTarget(MHWRender::MRenderTarget *target)
	{
		mColorTarget.target = target;
		mColorTargetChanged = true;
	}

	void setDepthTarget(MHWRender::MRenderTarget* target)
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

private:
	float mNear, mFar;
};
//
// Simple scene operation override to allow for clear color
// tracking.
//
class SimpleViewRenderSceneRender : public MHWRender::MSceneRender
{
public:
    SimpleViewRenderSceneRender(const MString &name, bool isPreUI);
	virtual ~SimpleViewRenderSceneRender(){	mTargets = NULL; mSimpleQuadRender = NULL;}
    MHWRender::MClearOperation & clearOperation() override;
	void setRenderTargets(MHWRender::MRenderTarget **targets, unsigned int targetCount);
    MHWRender::MRenderTarget* const* targetOverrideList(unsigned int &listSize) override;
    void postSceneRender(const MHWRender::MDrawContext & context) override;
	void setQuadRender(SimpleViewRenderQuadRender* quadRender){mSimpleQuadRender = quadRender;}
	MSceneFilterOption renderFilterOverride() override
	{
		return mIsPreUI ? kRenderPreSceneUIItems : kRenderShadedItems;
	}



protected:
	MHWRender::MRenderTarget **mTargets;
	unsigned int numTargets;
	SimpleViewRenderQuadRender* mSimpleQuadRender;
	bool mIsPreUI;

};

 class SimpleViewRenderSceneRenderUI : public MHWRender::MSceneRender
 {
 public:
	 SimpleViewRenderSceneRenderUI(const MString& name) :MSceneRender(name){};
	 virtual ~SimpleViewRenderSceneRenderUI(){}
	 MHWRender::MClearOperation& clearOperation() override { mClearOperation.setMask((unsigned int)MHWRender::MClearOperation::kClearNone); return mClearOperation; };
	
	 MSceneFilterOption renderFilterOverride() override
	 {
		 return kRenderPostSceneUIItems;
	 }
 };

#endif

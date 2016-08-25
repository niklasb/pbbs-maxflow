class PbbsSubrunsController < ApplicationController
  # GET /pbbs_subruns
  # GET /pbbs_subruns.xml
  def index
    @run_id = params[:run_id]
    @pbbs_subruns = PbbsSubrun.find(:all, :conditions => ["run_id=?", @run_id])
    @problem_id = @pbbs_subruns[0].pbbs_run.problem_id
    respond_to do |format|
      format.html # index.html.erb
      format.xml  { render :xml => @pbbs_subruns }
    end
  end

  # GET /pbbs_subruns/1
  # GET /pbbs_subruns/1.xml
  def show
    @pbbs_subrun = PbbsSubrun.find(params[:id])

    respond_to do |format|
      format.html # show.html.erb
      format.xml  { render :xml => @pbbs_subrun }
    end
  end

  # GET /pbbs_subruns/new
  # GET /pbbs_subruns/new.xml
  def new
    @pbbs_subrun = PbbsSubrun.new

    respond_to do |format|
      format.html # new.html.erb
      format.xml  { render :xml => @pbbs_subrun }
    end
  end

  # GET /pbbs_subruns/1/edit
  def edit
    @pbbs_subrun = PbbsSubrun.find(params[:id])
  end

  # POST /pbbs_subruns
  # POST /pbbs_subruns.xml
  def create
     
  end

  # PUT /pbbs_subruns/1
  # PUT /pbbs_subruns/1.xml
  def update
    
  end

  # DELETE /pbbs_subruns/1
  # DELETE /pbbs_subruns/1.xml
  def destroy
     
  end
end

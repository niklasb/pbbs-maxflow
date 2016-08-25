class PbbsHostsController < ApplicationController
  # GET /pbbs_hosts
  # GET /pbbs_hosts.xml
  def index
    @pbbs_hosts = PbbsHost.all

    respond_to do |format|
      format.html # index.html.erb
      format.xml  { render :xml => @pbbs_hosts }
    end
  end

  # GET /pbbs_hosts/1
  # GET /pbbs_hosts/1.xml
  def show
    @pbbs_host = PbbsHost.find(params[:id])

    respond_to do |format|
      format.html # show.html.erb
      format.xml  { render :xml => @pbbs_host }
    end
  end

  # GET /pbbs_hosts/new
  # GET /pbbs_hosts/new.xml
  def new
    @pbbs_host = PbbsHost.new

    respond_to do |format|
      format.html # new.html.erb
      format.xml  { render :xml => @pbbs_host }
    end
  end

  # GET /pbbs_hosts/1/edit
  def edit
    @pbbs_host = PbbsHost.find(params[:id])
  end

  # POST /pbbs_hosts
  # POST /pbbs_hosts.xml
  def create
    @pbbs_host = PbbsHost.new(params[:pbbs_host])

    respond_to do |format|
      if @pbbs_host.save
        format.html { redirect_to(@pbbs_host, :notice => 'Pbbs host was successfully created.') }
        format.xml  { render :xml => @pbbs_host, :status => :created, :location => @pbbs_host }
      else
        format.html { render :action => "new" }
        format.xml  { render :xml => @pbbs_host.errors, :status => :unprocessable_entity }
      end
    end
  end

  # PUT /pbbs_hosts/1
  # PUT /pbbs_hosts/1.xml
  def update
    @pbbs_host = PbbsHost.find(params[:id])

    respond_to do |format|
      if @pbbs_host.update_attributes(params[:pbbs_host])
        format.html { redirect_to(@pbbs_host, :notice => 'Pbbs host was successfully updated.') }
        format.xml  { head :ok }
      else
        format.html { render :action => "edit" }
        format.xml  { render :xml => @pbbs_host.errors, :status => :unprocessable_entity }
      end
    end
  end

  # DELETE /pbbs_hosts/1
  # DELETE /pbbs_hosts/1.xml
  def destroy
    @pbbs_host = PbbsHost.find(params[:id])
    PbbsHost.connection.execute("delete from pbbs_runs where host_id=%d"  %[@pbbs_host.id])
    @pbbs_host.destroy

    respond_to do |format|
      format.html { redirect_to(pbbs_hosts_url) }
      format.xml  { head :ok }
    end
  end
end

require 'test_helper'

class PbbsRunsControllerTest < ActionController::TestCase
  setup do
    @pbbs_run = pbbs_runs(:one)
  end

  test "should get index" do
    get :index
    assert_response :success
    assert_not_nil assigns(:pbbs_runs)
  end

  test "should get new" do
    get :new
    assert_response :success
  end

  test "should create pbbs_run" do
    assert_difference('PbbsRun.count') do
      post :create, :pbbs_run => @pbbs_run.attributes
    end

    assert_redirected_to pbbs_run_path(assigns(:pbbs_run))
  end

  test "should show pbbs_run" do
    get :show, :id => @pbbs_run.to_param
    assert_response :success
  end

  test "should get edit" do
    get :edit, :id => @pbbs_run.to_param
    assert_response :success
  end

  test "should update pbbs_run" do
    put :update, :id => @pbbs_run.to_param, :pbbs_run => @pbbs_run.attributes
    assert_redirected_to pbbs_run_path(assigns(:pbbs_run))
  end

  test "should destroy pbbs_run" do
    assert_difference('PbbsRun.count', -1) do
      delete :destroy, :id => @pbbs_run.to_param
    end

    assert_redirected_to pbbs_runs_path
  end
end
